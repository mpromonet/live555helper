cmake_minimum_required (VERSION 3.0.0)
 
project (live555helpertest)

set (CMAKE_CXX_STANDARD 11)

add_executable (${PROJECT_NAME} main.cpp)

set(LIVE ${CMAKE_BINARY_DIR}/live)
if (NOT EXISTS ${LIVE})
    file (DOWNLOAD http://www.live555.com/liveMedia/public/live555-latest.tar.gz ${CMAKE_BINARY_DIR}/live555.tar.gz )
    EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} -E tar xvf ${CMAKE_BINARY_DIR}/live555.tar.gz  RESULT_VARIABLE unpack_result)
    if(NOT unpack_result STREQUAL "0")
        message(FATAL_ERROR "Fetching live555 failed!")
    endif()
endif() 

FILE(GLOB LIVEHELPERSOURCE ${LIVE}/groupsock/*.c* ${LIVE}/liveMedia/*.c* ${LIVE}/UsageEnvironment/*.c* ${LIVE}/BasicUsageEnvironment/*.c* src/*.cpp)
add_library (liblive555helper STATIC ${LIVEHELPERSOURCE})	
target_include_directories(liblive555helper PUBLIC ${LIVE}/groupsock/include ${LIVE}/liveMedia/include ${LIVE}/UsageEnvironment/include ${LIVE}/BasicUsageEnvironment/include inc)
target_link_libraries (${PROJECT_NAME} liblive555helper) 

if (WIN32)
	add_definitions(-DNO_OPENSSL=1 -D_CRT_SECURE_NO_WARNINGS)
	target_link_libraries (${PROJECT_NAME} ws2_32)
else ()
	add_definitions(-DNO_OPENSSL=1 -DBSD=1 -DSOCKLEN_T=socklen_t -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE=1)
endif()
