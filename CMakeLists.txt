cmake_minimum_required (VERSION 3.0.0)
 
project (live555helpertest)

set (CMAKE_CXX_STANDARD 11)


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

if (WIN32)
	target_compile_definitions(liblive555helper PUBLIC NO_OPENSSL=1 _CRT_SECURE_NO_WARNINGS=1 NO_GETIFADDRS=1)
	target_link_libraries (liblive555helper ws2_32)
elseif (APPLE)
	target_compile_definitions(liblive555helper PUBLIC NO_OPENSSL=1 BSD=1 SOCKLEN_T=socklen_t _FILE_OFFSET_BITS=64 _LARGEFILE_SOURCE=1 NEED_XLOCALE_H=1)
else ()
	target_compile_definitions(liblive555helper PUBLIC NO_OPENSSL=1 BSD=1 SOCKLEN_T=socklen_t _FILE_OFFSET_BITS=64 _LARGEFILE_SOURCE=1)
endif()

add_executable (${PROJECT_NAME} main.cpp)
target_link_libraries (${PROJECT_NAME} liblive555helper) 

SET(CPACK_GENERATOR "TGZ")
SET(CPACK_PACKAGE_CONTACT "michel.promonet@free.fr")
INCLUDE(CPack)
