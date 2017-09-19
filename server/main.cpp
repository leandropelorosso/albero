#include <iostream>  

/*
#include <Windows.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <winsock2.h>
*/

#include <stdio.h>
#include <signal.h>
#include <stack>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/filestream.h"

#include "PCT.h"
#include "HCL.h"
#include "Albero2.h"
#include "ObservationReader.h"
#include "Color.h"
#include "Palette.h"
#include "ColorSchema.h"


#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>    //write

#include <thread>
#include <mutex> 

#include <IL/il.h>

mutex queue_mutex;

ColorSchema* PCT::numerical_forecast_schema;
ColorSchema* PCT::probabilistic_forecast_schema;
ColorSchema* PCT::mse_schema;
ColorSchema* PCT::bias_schema;

/*
#include "DevIL/include/IL/il.h"
#pragma comment( lib, "DevIL/DevIL.lib" )
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
*/
using namespace std;


//DWORD WINAPI PetitionHandler(void*);

//HANDLE server_thread;


Albero2 *albero2 = NULL;

string albero_images_path = "";
string cmorph_data_file = "";
string cmorph_index_file = "";
string reforecast_file_path = "";
string cmorph_data_folder = "";
int times_in_range = 4; // from the configuration, how many times (6 hour slot) in a range
int accumulation_ranges; // amount of accumulation ranges

int* csock = NULL;

volatile sig_atomic_t stop;

void die()
{
	cout << "[] Exiting..." << endl;
	exit(0);
}



struct RemoteRequest
{
public:
	// the socket handling the request
	int csock;
	// the request json
	string request_json;
};


stack <RemoteRequest> *requests_queue; /* Simple enough to create a stack */


void PetitionHandler(int client_sock) {

	//Receive a message from client
	cout << "Petition Handler" << endl;
	char client_message[655350];
	memset(client_message, 0, 655350);
	int read_size;
	
	while ((read_size = recv(client_sock, client_message, 655350, 0)) > 0)
	{
		RemoteRequest theRequest;
		theRequest.csock = client_sock;
		theRequest.request_json = string(client_message);

		queue_mutex.lock();
		requests_queue->push(theRequest);
		queue_mutex.unlock();
	}
}


string ProcessCommand(string cmd)
{
	string command;

	rapidjson::Document document;
	try {
		document.Parse(cmd.c_str());
	}
	catch (...)
	{
		cout << "ERROR: There was an error processing the setup file (setup.json). The json may be malformed or corrupt." << endl;
		return 0;
	}

	if (document.IsObject() && document.HasMember("action") && document["action"].IsString())
	{
		string command = document["action"].GetString();


		cout << command << endl;


		// ignore all actions if we are not initialized, except initialize.
		if (!albero2->initialized && command != "initialize" && command != "ping") { cout << "Initialize First!" << endl; return "NO"; }


		// ----------------------------------------------------------------------------------------------------
		// INITIALIZE
		// ----------------------------------------------------------------------------------------------------

		if (command == "initialize")
		{

			cout << "albero2->Initialize()";
			if (albero2 != NULL) {
				delete(albero2);
				albero2 = new Albero2();
			}

			cout << "  OK" << endl;


			string analog_range_from = document["configuration"]["analog-range-from"].GetString();
			string analog_range_to = document["configuration"]["analog-range-to"].GetString();
			string date = document["configuration"]["date"].GetString();
			string leadtime = document["configuration"]["leadtime-from"].GetString();
            string accumulation_range = document["configuration"]["accumulation-range"].GetString(); // for instance 24 hs
            string accumulation_ranges_str = document["configuration"]["accumulation-ranges"].GetString(); // for instance 3 (for a total of 72 hs)
			string analogs_amount = document["configuration"]["analogs-amount"].GetString();
			albero2->N_ANALOGS_PER_LAT_LON = stoi(analogs_amount);
			albero2->threshold_ranges.clear();

            times_in_range = stoi(accumulation_range) / 6; // how many times do we have in a range? Each time are 6 hours.
            accumulation_ranges = stoi(accumulation_ranges_str); // for instance: 3
			albero2->nAccumulationRanges = accumulation_ranges;

			/*
			if (document["configuration"].HasMember("threshold-ranges")){

			const rapidjson::Value& ranges = document["configuration"]["threshold-ranges"];

			for (int i = 0; i < document["configuration"]["threshold-ranges"].Size(); i++){
			const rapidjson::Value& range = document["configuration"]["threshold-ranges"][i];

			albero2->threshold_ranges.push_back(ThresholdRange(stoi(range[0].GetString()), stoi(range[1].GetString())));
			}
			}*/


			if (document["configuration"].HasMember("threshold-ranges")) {

				const rapidjson::Value& ranges = document["configuration"]["threshold-ranges"];

				for (int i = 0; i < (int)ranges.Size(); i++) {
					albero2->threshold_ranges.push_back(ThresholdRange(stoi(ranges[i].GetString()), 100));
				}
			}


            albero2->Initialize(stoi(date)*100);

			return albero2->stats->ToJSON();
		}


		// ----------------------------------------------------------------------------------------------------
		// PING
		// ---------------------------------------------------------------------------------------------------- 
		if (command == "ping")
		{
			return "pong";
		}


		// ----------------------------------------------------------------------------------------------------
		// GET OBSERVATION TILE
		// ----------------------------------------------------------------------------------------------------

		if (command == "get_observation")
		{
			string s_lat = document["y"].GetString();
			float y = ::atof(s_lat.c_str());

			string s_lon = document["x"].GetString();
			float x = ::atof(s_lon.c_str());

			string s_zoom = document["z"].GetString();
			float z = ::atof(s_zoom.c_str());

			string s_range_index = document["range_index"].GetString();
			int range_index = ::atoi(s_range_index.c_str());

            // obtenemos la fecha deseada si existe
            int date = (albero2->current_date);
            if(document.HasMember(("date"))){
                string s_date = document["date"].GetString();
                date = ::atoi(s_date.c_str());
            }

            int init_lat_pixel = (y + 1) * 256;
			int end_lat_pixel = y * 256;
			int init_lon_pixel = x * 256;
			int end_lon_pixel = (x + 1) * 256;

			cout << "REQ>>NumForecast>> " << x << ", " << y << ", " << z << endl;

			/* Grabamos los valores interpolados */
			float *observation_values = ObservationReader::GetInterpolatedValuesLambert(date, range_index, init_lat_pixel, init_lon_pixel, end_lat_pixel, end_lon_pixel, 256, 256, z);
			if (observation_values == NULL) {
				return "none";
			}
			PCT::SelectScale(-3, true);


			string probabilistic_forecast_image_filename = albero_images_path + "probabilistic_forecast_" + to_string((int)x) + "_" + to_string((int)y) + "_" + to_string((int)z) + ".png";
            WriteImage(PCT::numerical_forecast_schema, observation_values, 256, 256, probabilistic_forecast_image_filename, 0.0f, 60.0f);

			delete(observation_values);

			return probabilistic_forecast_image_filename;
		}

		// ----------------------------------------------------------------------------------------------------
		// GET PROBABILISTIC FORECAST TILE
		// ----------------------------------------------------------------------------------------------------

		if (command == "get_forecast")
		{
			string s_lat = document["y"].GetString();
			float y = ::atof(s_lat.c_str());

			string s_lon = document["x"].GetString();
			float x = ::atof(s_lon.c_str());

			string s_zoom = document["z"].GetString();
			float z = ::atof(s_zoom.c_str());

			string s_threshold_index = document["threshold_index"].GetString();
			int threshold_index = ::atoi(s_threshold_index.c_str());

			string s_range_index = document["range_index"].GetString();
			int range_index = ::atoi(s_range_index.c_str());

			float init_lat_pixel = (y + 1) * 256;
			float end_lat_pixel = y * 256;
			float init_lon_pixel = x * 256;
			float end_lon_pixel = (x + 1) * 256;

			cout << "Procesando pedido de " << x << ", " << y << ", " << z << endl;

			/* Grabamos los valores interpolados */
			size_t probability_map_index = range_index * albero2->threshold_ranges.size() + threshold_index;

			float* interpolated_values = Interpolate8(albero2->probability_map[probability_map_index], albero2->probability_map_width, albero2->probability_map_height,
				albero2->forecasts->lats[0],
				albero2->forecasts->lons[0],
				albero2->forecasts->lats[albero2->forecasts->NLAT - 1],
				albero2->forecasts->lons[albero2->forecasts->NLON - 1],
				init_lat_pixel, init_lon_pixel, end_lat_pixel, end_lon_pixel, 256, 256, z);

			PCT::SelectScale(-7, true);

			string probabilistic_forecast_image_filename = albero_images_path + "probabilistic_forecast_" + to_string((int)x) + "_" + to_string((int)y) + "_" + to_string((int)z) + ".png";
            WriteImage(PCT::probabilistic_forecast_schema, interpolated_values, 256, 256, probabilistic_forecast_image_filename, 0,/*1.0f/ albero2->N_ANALOGS_PER_LAT_LON*/ 100.0f);
			delete(interpolated_values);

			return probabilistic_forecast_image_filename;
		}

		// ----------------------------------------------------------------------------------------------------
		// GET NUMERICAL FORECAST TILE (FOR THE HISTORIC FORECAST)
		// ----------------------------------------------------------------------------------------------------
   //   historical_forecast_index_by_range_and_date[range][date]


              if (command == "get_num_forecast")
              {
                  string s_lat = document["y"].GetString();
                  float y = ::atof(s_lat.c_str());

                  string s_lon = document["x"].GetString();
                  float x = ::atof(s_lon.c_str());

                  string s_zoom = document["z"].GetString();
                  float z = ::atof(s_zoom.c_str());

                  string s_range_index = document["range_index"].GetString();
                  int range_index = ::atoi(s_range_index.c_str());

                  // obtenemos la fecha deseada si existe
                  int current_date = (albero2->current_date);
                  int requested_date = current_date;

                  if(document.HasMember(("date"))){
                      string s_date = document["date"].GetString();
                      requested_date = ::atoi(s_date.c_str());
                  }

                  int init_lat_pixel = (y + 1) * 256;
                  int end_lat_pixel = y * 256;
                  int init_lon_pixel = x * 256;
                  int end_lon_pixel = (x + 1) * 256;

                  cout << "REQ>>NumForecast>> " << x << ", " << y << ", " << z << endl;

                  // Grabamos los valores interpolados

                  // Retrieve the forecast for the selected date [NLATxNLON]
                  float* forecast = &albero2->forecasts->forecasts_by_range[range_index][albero2->forecasts->forecast_index[requested_date]];

               //   if(current_date!=requested_date){
                     // int historical_forecast_index = albero2->historical_forecast_index_by_range_and_date[range_index][requested_date];
                      //forecast = &albero2->historic_forecast_by_range[range_index][historical_forecast_index];
             //     }else{
                   //   forecast = albero2->current_forecast_by_range[range_index];
            //      }

                  float* interpolated_values = Interpolate8(forecast, (int)albero2->forecasts->NLON, (int)albero2->forecasts->NLAT,
                      albero2->forecasts->lats[0],
                      albero2->forecasts->lons[0],
                      albero2->forecasts->lats[albero2->forecasts->NLAT - 1],
                      albero2->forecasts->lons[albero2->forecasts->NLON - 1],
                      init_lat_pixel, init_lon_pixel, end_lat_pixel, end_lon_pixel, 256, 256, z);


                  PCT::SelectScale(-3, true);
                  string probabilistic_forecast_image_filename = albero_images_path + "num_forecast_" + to_string((int)x) + "_" + to_string((int)y) + "_" + to_string((int)z) + ".png";
                  cout << probabilistic_forecast_image_filename << endl;

                  WriteImage(PCT::numerical_forecast_schema, interpolated_values, 256, 256, probabilistic_forecast_image_filename, 0,60);
                  delete(interpolated_values);

                  return probabilistic_forecast_image_filename;
              }




              /*
		if (command == "get_num_forecast")
		{
			string s_lat = document["y"].GetString();
			float y = ::atof(s_lat.c_str());

			string s_lon = document["x"].GetString();
			float x = ::atof(s_lon.c_str());

			string s_zoom = document["z"].GetString();
			float z = ::atof(s_zoom.c_str());

			string s_range_index = document["range_index"].GetString();
			int range_index = ::atoi(s_range_index.c_str());

			int init_lat_pixel = (y + 1) * 256;
			int end_lat_pixel = y * 256;
			int init_lon_pixel = x * 256;
			int end_lon_pixel = (x + 1) * 256;

			cout << "REQ>>NumForecast>> " << x << ", " << y << ", " << z << endl;

            // Grabamos los valores interpolados
			float* interpolated_values = Interpolate8(albero2->current_forecast_by_range[range_index], (int)albero2->forecasts->NLON, (int)albero2->forecasts->NLAT,
				albero2->forecasts->lats[0],
				albero2->forecasts->lons[0],
				albero2->forecasts->lats[albero2->forecasts->NLAT - 1],
				albero2->forecasts->lons[albero2->forecasts->NLON - 1],
				init_lat_pixel, init_lon_pixel, end_lat_pixel, end_lon_pixel, 256, 256, z);


			PCT::SelectScale(-3, true);
			string probabilistic_forecast_image_filename = albero_images_path + "num_forecast_" + to_string((int)x) + "_" + to_string((int)y) + "_" + to_string((int)z) + ".png";
			cout << probabilistic_forecast_image_filename << endl;

            WriteImage(interpolated_values, 256, 256, probabilistic_forecast_image_filename, 0,60);
			delete(interpolated_values);

			return probabilistic_forecast_image_filename;
        }*/



		// ----------------------------------------------------------------------------------------------------
		// GET MEAN SQUARED ERROR TILE
		// ----------------------------------------------------------------------------------------------------
		if (command == "get_mse")
		{
			string s_lat = document["y"].GetString();
			float y = ::atof(s_lat.c_str());

			string s_lon = document["x"].GetString();
			float x = ::atof(s_lon.c_str());

			string s_zoom = document["z"].GetString();
			float z = ::atof(s_zoom.c_str());

			string s_range_index = document["range_index"].GetString();
			int range_index = ::atoi(s_range_index.c_str());

			int init_lat_pixel = (y + 1) * 256;
			int end_lat_pixel = y * 256;
			int init_lon_pixel = x * 256;
			int end_lon_pixel = (x + 1) * 256;

			cout << "Procesando pedido de " << x << ", " << y << ", " << z << endl;

			/* Grabamos los valores interpolados */
			float* interpolated_values = Interpolate8(albero2->mean_square_error_map[range_index], (int)albero2->forecasts->NLON, (int)albero2->forecasts->NLAT,
				albero2->forecasts->lats[0],
				albero2->forecasts->lons[0],
				albero2->forecasts->lats[albero2->forecasts->NLAT - 1],
				albero2->forecasts->lons[albero2->forecasts->NLON - 1],
				init_lat_pixel, init_lon_pixel, end_lat_pixel, end_lon_pixel, 256, 256, z);

			PCT::SelectScale(-6, true);
			string probabilistic_forecast_image_filename = albero_images_path + "mse" + to_string((int)x) + "_" + to_string((int)y) + "_" + to_string((int)z) + ".png";
            WriteImage(PCT::mse_schema, interpolated_values, 256, 256, probabilistic_forecast_image_filename, 0, 20);
			delete(interpolated_values);

			return probabilistic_forecast_image_filename;
		}


		// ----------------------------------------------------------------------------------------------------
		// GET ANALOG VIEWER
		// ----------------------------------------------------------------------------------------------------
		if (command == "get_analog_viewer")
		{
			string s_lat = document["lat"].GetString();
			float lat = ::atof(s_lat.c_str());

			string s_lon = document["lon"].GetString();
			float lon = ::atof(s_lon.c_str());

			string s_range_index = document["range_index"].GetString();
			int range_index = ::atoi(s_range_index.c_str());


			if (lat < albero2->forecasts->lats[0] + 1 ||
				lat > albero2->forecasts->lats[albero2->forecasts->NLAT - 1] - 1 ||
				lon < albero2->forecasts->lons[0] + 1 ||
				lon > albero2->forecasts->lons[albero2->forecasts->NLON - 1] - 1) {
				return "[]";
			}

			AnalogsResponse* analogs = albero2->RenderAnalogForecasts(lat, lon, range_index);

			string result = "[" + analogs->ToJSON() + "]";

			delete(analogs);

			// return the result
			return result;
		}

	}

	return "";
}


void ProcessQueue() {

	while (true)
	{
		//if( requests_queue->size()>0)cout<< requests_queue->size() << endl;

		if (!requests_queue->empty())
		{
			queue_mutex.lock();
			RemoteRequest theRequest = requests_queue->top();
			requests_queue->pop();
			queue_mutex.unlock();
			
			cout << theRequest.request_json << endl;
			string result = ProcessCommand(theRequest.request_json.c_str());
			if (result == "") result = "NO_RESPONSE";

			size_t bytecount = 0;
			if (write(theRequest.csock, result.c_str(), (int)result.length()) <= 0) {
				fprintf(stderr, "Error sending data\n");
			}

			close(theRequest.csock);
			//	free(theRequest.csock);

		}
	}
	return;

}



void dispose() {
	cout << "[] Releasing memory..." << endl;
	if (albero2 != NULL) {
		delete(albero2);
	}
	Color::Dispose();
	ObservationReader::Dispose();
	//TerminateThread(server_thread, 0);
	//CloseHandle(server_thread);
	delete(requests_queue);
	if (csock != NULL)delete(csock);
}

void halt_handler(int s) {
	dispose();
	die();
}

float *ObservationReader::file_data = NULL;

int main(int argc, char* argv[])
{
	if (argc != 4) {
		cout << "Error, please specify parameters." << endl;
		return 0;
	}

 //   ilInit();



        /*Palette::init();
        Palette::load("C:/albero_color_palettes/blue_clear.png");  // 0
        Palette::load("C:/albero_color_palettes/blue_deep.png"); // 1
        Palette::load("C:/albero_color_palettes/blue_ice.png"); // 2
        Palette::load("C:/albero_color_palettes/blue_sky.png"); // 3
        Palette::load("C:/albero_color_palettes/blue_spectrum.png"); // 4
        Palette::load("C:/albero_color_palettes/blue_water.png"); // 5
        Palette::load("C:/albero_color_palettes/redsky.png"); // 6
*/

	// Listen for CTRL-C
	struct sigaction sigIntHandler;
	sigIntHandler.sa_handler = halt_handler;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;
	sigaction(SIGINT, &sigIntHandler, NULL);
	
	// Initialize
	cout << "[] Iniciando" << endl;

	albero_images_path = argv[1];
	cmorph_data_folder = argv[2];
	reforecast_file_path = argv[3];

	
    cmorph_index_file = cmorph_data_folder + "/index3.txt";
    cmorph_data_file = cmorph_data_folder + "/cmorph3.dat";

	albero2 = new Albero2();

    //PCT::numerical_forecast_schema = new Palette("/home/vertexar/projects/alberoserver.linux/data/escala_laura.png");
    //PCT::probabilistic_forecast_schema = new Palette("/home/vertexar/projects/alberoserver.linux/data/escala_probabilidad_laura.png");

    PCT::numerical_forecast_schema = (new hcl(Vector2(80.0f, 1.36f), Vector2(231.00000000000003f, 0.16797619047619047f)))->discretization(5);
    PCT::probabilistic_forecast_schema = (new hcl(Vector2(65.57142857142857f, 1.2729761904761905f), Vector2(351.0f, 0.3096428571428571f)))->discretization(10);
    PCT::mse_schema = (new hcl(Vector2(93.00000000000001f, 1.3498809523809523f), Vector2(10.714285714285714f, 0.9694047619047619f)))->discretization(2);
    PCT::bias_schema = new HCLDiverging();

	cout << "[] ObservationReader::Init()" << endl;
	ObservationReader::Init();


    //PCT::SelectScale(-3, true);
    PCT::numerical_forecast_schema->render_scale(10, 200, 0, 60, albero_images_path + "numerical_forecast_big_scale.png");
    //PCT::SelectScale(-3, false);
    PCT::numerical_forecast_schema->render_scale(10, 152, 0, 60, albero_images_path + "numerical_forecast_small_scale.png");
    //PCT::SelectScale(3, false);
    PCT::bias_schema->render_scale(10, 152, -60, 60, albero_images_path + "bias_small_scale.png");
    //PCT::SelectScale(-6, true);
    PCT::mse_schema->render_scale(10, 152, 0, 20, albero_images_path + "mse_big_scale.png");
    //PCT::SelectScale(-7, true);
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
	/*
	if (setsockopt(socket_desc, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(int)) < 0) {
		perror("setsockopt");
		exit(1);
	}*/
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

			//	std::thread thread_process_queue(PetitionHandler, client_sock);
		}
	}
}


