#include <iostream>
#include <stdio.h>
#include <signal.h>
#include <stack>
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>    //write
#include <thread>
#include <mutex>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/filestream.h"

#include "pct.h"
#include "hcl.h"
#include "albero2.h"
#include "observation_reader.h"
#include "color.h"
#include "palette.h"
#include "color_schema.h"
#include "commands/command.h"

#include <IL/il.h>

#include "forecast_importer.h"


mutex queue_mutex;

ColorSchema* PCT::numerical_forecast_schema;
ColorSchema* PCT::probabilistic_forecast_schema;
ColorSchema* PCT::mse_schema;
ColorSchema* PCT::bias_schema;

using namespace std;

Albero2 *albero2 = NULL;

string albero_images_path = "";
string cmorph_data_file = "";
string cmorph_index_file = "";
string reforecast_file_path = "";
string cmorph_data_folder = "";

int* csock = NULL;
volatile sig_atomic_t stop;

// Request structure containing socket and request json.
struct RemoteRequest
{
public:
	// the socket handling the request
	int csock;
	// the request json
	string request_json;
};

// Stack of Requests
stack <RemoteRequest> *requests_queue;

// Just Exit
void die()
{
    cout << "[] Exiting..." << endl;
    exit(0);
}

// Process a socket request, adding it to the queue
void PetitionHandler(int client_sock) {

    // Receive a message from client
	cout << "Petition Handler" << endl;
	char client_message[655350];
	memset(client_message, 0, 655350);
	int read_size;

    // While there is something to read.
	while ((read_size = recv(client_sock, client_message, 655350, 0)) > 0)
	{
        // Create request
		RemoteRequest theRequest;
		theRequest.csock = client_sock;
		theRequest.request_json = string(client_message);

        // Add to Queue
		queue_mutex.lock();
		requests_queue->push(theRequest);
		queue_mutex.unlock();
	}
}

// Commands
Commands::Command* cmd_ping = new Commands::Ping();
Commands::Command* cmd_get_observation = new Commands::GetObservation();
Commands::Command* cmd_get_probabilistic_forecast = new Commands::GetProbabilisticForecast();
Commands::Command* cmd_get_numerical_forecast = new Commands::GetNumericalForecast();
Commands::Command* cmd_get_mse = new Commands::GetMSE();
Commands::Command* cmd_get_analog_viewer = new Commands::GetAnalogViewer();
Commands::Command* cmd_initialize = new Commands::Initialize();
Commands::Command* cmd_get_available_days = new Commands::GetAvailableDays();

// Process a given command
string ProcessCommand(string cmd_json)
{
    // parse the command json
    rapidjson::Document document;
	try {
        document.Parse(cmd_json.c_str());
	}
	catch (...)
	{
		cout << "ERROR: There was an error processing the setup file (setup.json). The json may be malformed or corrupt." << endl;
		return 0;
	}

    // if there is an action defined on the command json.
	if (document.IsObject() && document.HasMember("action") && document["action"].IsString())
	{
        string command = document["action"].GetString();
		cout << command << endl;

		// ignore all actions if we are not initialized, except initialize.
        if (!albero2->initialized && command != "initialize" && command != "ping" && command != "get_available_days") { cout << "Initialize First!" << endl; return "NO"; }

        // INITIALIZE
		if (command == "initialize")
		{
            cout << "albero2->Initialize()";
            if (albero2 != NULL) {
                delete(albero2);
                albero2 = new Albero2();
            }
            cout << "  OK" << endl;
            return cmd_initialize->Execute(albero2, document);
		}

        // PING
        if (command == "ping") return cmd_ping->Execute(albero2, document);

        // GET OBSERVATION TILE
        if (command == "get_observation") return cmd_get_observation->Execute(albero2, document);

		// GET PROBABILISTIC FORECAST TILE
        if (command == "get_forecast") return cmd_get_probabilistic_forecast->Execute(albero2, document);

		// GET NUMERICAL FORECAST TILE (FOR THE HISTORIC FORECAST)
        if (command == "get_num_forecast") return cmd_get_numerical_forecast->Execute(albero2, document);

		// GET MEAN SQUARED ERROR TILE
        if (command == "get_mse") return cmd_get_mse->Execute(albero2, document);

		// GET ANALOG VIEWER
        if (command == "get_analog_viewer") return cmd_get_analog_viewer->Execute(albero2, document);

        // GET ANALOG VIEWER
        if (command == "get_available_days") return cmd_get_available_days->Execute(albero2, document);
	}

    // return a default empty response
	return "";
}


// Process request queue, one request at a time
void ProcessQueue() {

	while (true)
	{
		if (!requests_queue->empty())
		{
			queue_mutex.lock();
			RemoteRequest theRequest = requests_queue->top();
			requests_queue->pop();
			queue_mutex.unlock();
			
			cout << theRequest.request_json << endl;
            string result = ProcessCommand(theRequest.request_json.c_str()) + "\n";
            if (result == "") result = "NO_RESPONSE\n";

			size_t bytecount = 0;
			if (write(theRequest.csock, result.c_str(), (int)result.length()) <= 0) {
				fprintf(stderr, "Error sending data\n");
			}
			close(theRequest.csock);
		}
	}
	return;
}

// Release memory
void dispose() {
	cout << "[] Releasing memory..." << endl;
	if (albero2 != NULL) {
		delete(albero2);
	}
	Color::Dispose();	
	delete(requests_queue);
	if (csock != NULL)delete(csock);
}

// Halt Handler for CTRL+C
void halt_handler(int s) {
	dispose();
	die();
}

// Main
int main(int argc, char* argv[])
{

        if(argc==2 && std::string(argv[1]) == "-v"){
            std::cout << "2.0.9" << endl;
            return 0;
        }

        // Lets check if we are trying to get in the import mode
        if(argc == 4){
            if(std::string(argv[1]) == "-import"){

                // get files paths
                std::string import_file = std::string(argv[2]);
                std::string collection_file = std::string(argv[3]);

                // import
                ForecastImporter* imp = new ForecastImporter();
                imp->Import(import_file, collection_file);
                delete(imp);
                return 0;
            }
        }

        if (argc != 4) {
            cout << "Error, please specify parameters." << endl;
            return 0;
        }

	// Listen for CTRL-C
	struct sigaction sigIntHandler;
	sigIntHandler.sa_handler = halt_handler;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;
	sigaction(SIGINT, &sigIntHandler, NULL);
	
	// Initialize
	cout << "[] Iniciando" << endl;

    // Read parameters
	albero_images_path = argv[1];
	cmorph_data_folder = argv[2];
	reforecast_file_path = argv[3];

    cmorph_index_file = cmorph_data_folder + "/index3.txt";
    cmorph_data_file = cmorph_data_folder + "/cmorph3.dat";

    // Initialize Albero
	albero2 = new Albero2();

    // Create the color scales

    //PCT::numerical_forecast_schema = new Palette("/home/vertexar/projects/alberoserver.linux/data/escala_laura.png");
    //PCT::probabilistic_forecast_schema = new Palette("/home/vertexar/projects/alberoserver.linux/data/escala_probabilidad_laura.png");
    PCT::numerical_forecast_schema = (new hcl(Vector2(80.0f, 1.36f), Vector2(231.00000000000003f, 0.16797619047619047f)))->discretization(5);
    PCT::probabilistic_forecast_schema = (new hcl(Vector2(65.57142857142857f, 1.2729761904761905f), Vector2(351.0f, 0.3096428571428571f)))->discretization(10);
    PCT::mse_schema = (new hcl(Vector2(93.00000000000001f, 1.3498809523809523f), Vector2(10.714285714285714f, 0.9694047619047619f)))->discretization(2);
    PCT::bias_schema = new HCLDiverging(); 

    // Render color scales to be used on the front end
    PCT::numerical_forecast_schema->render_scale(10, 200, 0, 60, albero_images_path + "numerical_forecast_big_scale.png");
    PCT::numerical_forecast_schema->render_scale(10, 152, 0, 60, albero_images_path + "numerical_forecast_small_scale.png");
    PCT::bias_schema->render_scale(10, 152, -60, 60, albero_images_path + "bias_small_scale.png");
    PCT::mse_schema->render_scale(10, 152, 0, 20, albero_images_path + "mse_big_scale.png");
    PCT::probabilistic_forecast_schema->render_scale(10, 152, 0, 100, albero_images_path + "probabilistic_forecast_big_scale.png");

    cout << "[] Scales Rendered." << endl;

	/********* QUEUE THREAD *********************/
	requests_queue = new stack<RemoteRequest>();

	// Elsewhere in some part of the galaxy
	std::thread t1(ProcessQueue);
	

	/****************** SERVER ******************************************/	
	int socket_desc, client_sock, c;
	struct sockaddr_in server, client;
	

	//Create socket
	socket_desc = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_desc == -1)
	{
		printf("Could not create socket");
	}
	puts("Socket created");

	int enable = 1;
	if (setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
		perror("setsockopt");
		exit(1);
	}

	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(1105);

	//Bind
	if (bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0)
	{
		//print the error message
		perror("bind failed. Error");
		return 1;
	}
	puts("bind done");

	//Listen
	listen(socket_desc, 3);

	//Accept and incoming connection
	puts("Waiting for incoming connections...");
	c = sizeof(struct sockaddr_in);

	while (true) {

		//accept connection from an incoming client
		client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);

		if (client_sock >= 0)
		{
			cout << "pedido recibido" << endl;
			std::thread t2(PetitionHandler, client_sock);
			t2.join();
		}
	}
}


