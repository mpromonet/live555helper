cmake_minimum_required (VERSION 3.0.0)
 
project (live555helpertest)

set (CMAKE_CXX_STANDARD 11)

add_executable (${PROJECT_NAME} main.cpp)

if (NOT EXISTS live)
    file (DOWNLOAD http://www.live555.com/liveMedia/public/live555-latest.tar.gz ${CMAKE_SOURCE_DIR}/live555-latest.tar.gz )
    EXECUTE_PROCESS(COMMAND 7z x live555-latest.tar.gz -so COMMAND 7z x -aoa -ttar -si)
endif(NOT EXISTS live) 

FILE(GLOB LIVEHELPERSOURCE live/groupsock/*.c* live/liveMedia/*.c* live/UsageEnvironment/*.c* live/BasicUsageEnvironment/*.c* src/*.cpp)
add_library (liblive555helper STATIC ${LIVEHELPERSOURCE})	
target_include_directories(liblive555helper PUBLIC live/groupsock/include live/liveMedia/include live/UsageEnvironment/include live/BasicUsageEnvironment/include inc)
target_link_libraries (${PROJECT_NAME} liblive555helper) 

if (MSVC)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	target_link_libraries (${PROJECT_NAME} ws2_32)
else (MSVC)
	add_definitions(-DBSD=1 -DSOCKLEN_T=socklen_t -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE=1)
endif(MSVC)
