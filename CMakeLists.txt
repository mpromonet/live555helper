cmake_minimum_required (VERSION 3.10)
 
project (live555helpertest)

set (CMAKE_CXX_STANDARD 20)
set (DETECT_OPENSSL "ON" CACHE STRING "Detect OPENSSL")

set(LIVE ${CMAKE_BINARY_DIR}/live)
if (NOT EXISTS ${LIVE})
    file (DOWNLOAD http://www.live555.com/liveMedia/public/live555-latest.tar.gz ${CMAKE_BINARY_DIR}/live555.tar.gz )
    execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${LIVE})
    execute_process(COMMAND ${CMAKE_COMMAND} -E tar xvf ${CMAKE_BINARY_DIR}/live555.tar.gz WORKING_DIRECTORY ${CMAKE_BINARY_DIR} RESULT_VARIABLE unpack_result)
    if(NOT unpack_result EQUAL "0")
        message(FATAL_ERROR "Fetching live555 failed!")
    endif()
endif() 

FILE(GLOB LIVEHELPERSOURCE ${LIVE}/groupsock/*.c* ${LIVE}/liveMedia/*.c* ${LIVE}/UsageEnvironment/*.c* ${LIVE}/BasicUsageEnvironment/*.c* src/*.cpp)
add_library (liblive555helper STATIC ${LIVEHELPERSOURCE})	
target_include_directories(liblive555helper PUBLIC ${LIVE}/groupsock/include ${LIVE}/liveMedia/include ${LIVE}/UsageEnvironment/include ${LIVE}/BasicUsageEnvironment/include inc)

if (WIN32)
	target_compile_definitions(liblive555helper PUBLIC _CRT_SECURE_NO_WARNINGS=1 NO_GETIFADDRS=1 NO_STD_LIB=1)
	target_link_libraries (liblive555helper PUBLIC ws2_32)
elseif (APPLE)
	target_compile_definitions(liblive555helper PUBLIC BSD=1 SOCKLEN_T=socklen_t _FILE_OFFSET_BITS=64 _LARGEFILE_SOURCE=1 NO_STD_LIB=1 NEED_XLOCALE_H=1)
else ()
	target_compile_definitions(liblive555helper PUBLIC BSD=1 SOCKLEN_T=socklen_t _FILE_OFFSET_BITS=64 _LARGEFILE_SOURCE=1 NO_STD_LIB=1)
endif()

# openssl ?
if (DETECT_OPENSSL) 
    find_package(OpenSSL QUIET)
endif()
MESSAGE("OpenSSL_FOUND = ${OpenSSL_FOUND}")
if (OpenSSL_FOUND)
    target_include_directories(liblive555helper PUBLIC ${OPENSSL_INCLUDE_DIR})
    target_link_libraries(liblive555helper PUBLIC ${OPENSSL_LIBRARIES})    
else()
    target_compile_definitions(liblive555helper PUBLIC NO_OPENSSL=1)
endif()

add_executable (${PROJECT_NAME} main.cpp)
target_link_libraries (${PROJECT_NAME} liblive555helper) 

SET(CPACK_GENERATOR "TGZ")
SET(CPACK_PACKAGE_CONTACT "michel.promonet@free.fr")
INCLUDE(CPack)
