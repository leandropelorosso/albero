set (CMAKE_CXX_STANDARD 11)
cmake_minimum_required(VERSION 2.6)
project(albero)

#link_directories(/home/silencer/Desktop/squirrel)
#include_directories(/home/silencer/Desktop/squirrel/includes ${CMAKE_CURRENT_SOURCE_DIR}/include)

#file(GLOB SPINE_SRC "spine/*.c") # add spine sources
file(GLOB MAIN_SRC "*.cpp") # add spine sources
file(GLOB CMDS_SRC "commands/*.cpp") # add spine sources
file(GLOB CMDS_HDR "commands/*.h") # add spine sources

add_executable(albero ${MAIN_SRC} ${CMDS_SRC} ${CMDS_HDR})

install(TARGETS albero RUNTIME DESTINATION bin)
TARGET_LINK_LIBRARIES(albero netcdf IL pthread)

