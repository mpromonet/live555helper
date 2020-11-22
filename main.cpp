/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** main.cpp
** 
** Simple Test Using live555helper
** -------------------------------------------------------------------------*/

#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <signal.h>
#include "environment.h"
#include "rtspconnectionclient.h"
#include "sdpclient.h"
#include "mkvclient.h"


class RTSPCallback : public RTSPConnection::Callback
{
	private:
		std::map<std::string,std::ofstream> m_ofs;
		std::string m_fileprefix;
		
	public:
		RTSPCallback(const std::string & output) : m_fileprefix(output)  {}
		
		virtual bool    onNewSession(const char* id, const char* media, const char* codec, const char*) {
			if (!m_fileprefix.empty()) {
				auto it = m_ofs.find(id);
				if (it == m_ofs.end()) {
					std::string filename = m_fileprefix + "_" + media + "_" + codec + "_" + id;
					m_ofs[id].open(filename.c_str(), std::ofstream::out | std::ofstream::trunc);
				}
			}
			std::cout << id << " " << media << "/" <<  codec << std::endl;
			return true;
		}
		
		virtual bool    onData(const char* id, unsigned char* buffer, ssize_t size, struct timeval presentationTime) {
			std::cout << id << " " << size << " ts:" << presentationTime.tv_sec << "." << presentationTime.tv_usec << std::endl;
			auto it = m_ofs.find(id);
			if (it != m_ofs.end()) {
				it->second.write((char*)buffer, size);
			}
			return true;
		}
		
		virtual void    onError(RTSPConnection& connection, const char* message) {
			std::cout << "Error:" << message << std::endl;
			connection.start(10);
		}
		
		virtual void    onConnectionTimeout(RTSPConnection& connection) {
			std::cout << "Connection timeout -> retry" << std::endl;
			connection.start();
		}
		
		virtual void    onDataTimeout(RTSPConnection& connection)       {
			std::cout << "Data timeout -> retry" << std::endl;
			connection.start();
		}		
};

class SDPCallback : public SDPClient::Callback
{
	private:
		std::map<std::string,std::ofstream> m_ofs;
		std::string m_fileprefix;
		
	public:
		SDPCallback(const std::string & output) : m_fileprefix(output)  {}
		
		virtual bool    onNewSession(const char* id, const char* media, const char* codec, const char*) {
			if (!m_fileprefix.empty()) {
				auto it = m_ofs.find(id);
				if (it == m_ofs.end()) {
					std::string filename = m_fileprefix + "_" + media + "_" + codec + "_" + id;
					m_ofs[id].open(filename.c_str(), std::ofstream::out | std::ofstream::trunc);
				}
			}
			std::cout << id << " " << media << "/" <<  codec << std::endl;
			return true;
		}
				
		virtual bool    onData(const char* id, unsigned char* buffer, ssize_t size, struct timeval presentationTime) {
			std::cout << id << " " << size << " ts:" << presentationTime.tv_sec << "." << presentationTime.tv_usec << std::endl;
			auto it = m_ofs.find(id);
			if (it != m_ofs.end()) {
				it->second.write((char*)buffer, size);
			}
			return true;
		}
		
		virtual void    onError(SDPClient& connection, const char* message) {
			std::cout << "Error:" << message << std::endl;
		}		
};

class MKVCallback : public MKVClient::Callback
{
	private:
		std::map<std::string,std::ofstream> m_ofs;
		std::string m_fileprefix;
		
	public:
		MKVCallback(const std::string & output) : m_fileprefix(output)  {}
		
		virtual bool    onNewSession(const char* id, const char* media, const char* codec, const char*) {
			if (!m_fileprefix.empty()) {
				auto it = m_ofs.find(id);
				if (it == m_ofs.end()) {
					std::string filename = m_fileprefix + "_" + media + "_" + codec + "_" + id;
					m_ofs[id].open(filename.c_str(), std::ofstream::out | std::ofstream::trunc);
				}
			}
			std::cout << id << " " << media << "/" <<  codec << std::endl;
			return true;
		}
		
		virtual bool    onData(const char* id, unsigned char* buffer, ssize_t size, struct timeval presentationTime) {
			std::cout << id << " " << size << " ts:" << presentationTime.tv_sec << "." << presentationTime.tv_usec << std::endl;
			auto it = m_ofs.find(id);
			if (it != m_ofs.end()) {
				it->second.write((char*)buffer, size);
			}
			return true;
		}
};

char stop = 0;
void sig_handler(int signo)
{
	if (signo == SIGINT) {
		printf("received SIGINT\n");
		stop = 1;
	}
}

void usage(const char* app) {
	std::cout << "Usage: " << app << " url" << std::endl;
}

int main(int argc, char *argv[])
{
	// default value
	int  timeout = 10;
	int rtptransport = RTSPConnection::RTPUDPUNICAST;
	int  logLevel = 255;
	std::string output;
	
	// decode args
	int c = 0;
	while ((c = getopt (argc, argv, "hv:" "t:o:" "MTH")) != -1)
	{
		switch (c)
		{
			case 'v':	logLevel   = atoi(optarg);  break;
			case 'h':	usage(argv[0]);  return 0;
			
			case 't':	timeout= atoi(optarg);  break;
			case 'o':	output = optarg;  break;
			
			case 'M':	rtptransport = RTSPConnection::RTPUDPMULTICAST;  break;
			case 'T':	rtptransport = RTSPConnection::RTPOVERTCP;  break;
			case 'H':	rtptransport = RTSPConnection::RTPOVERHTTP;  break;
		}
	}
	
	if (optind<argc)
	{
		Environment env(stop);
		std::string url = argv[optind];
		if (url.find("rtsp://") == 0) {
			RTSPCallback cb(output);
			RTSPConnection rtspClient(env, &cb, url.c_str(), timeout, rtptransport, logLevel);
			
			signal(SIGINT, sig_handler);
			std::cout << "Start mainloop" << std::endl;
			env.mainloop();	
			
		} else if (url.find("file://") == 0) {
			MKVCallback cb(output);
			MKVClient mkvClient(env, &cb, url.c_str(), std::map<std::string,std::string>());
			
			signal(SIGINT, sig_handler);
			std::cout << "Start mainloop" << std::endl;
			env.mainloop();	
			
		} else {
			std::string sdp;
			if (url.find("rtp://") == 0) {
				sdp.assign(SDPClient::getSdpFromRtpUrl(url));
			} else {		
				std::ifstream is(url);
				sdp.assign((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());
			}
			std::cout << "SDP:\n" << sdp << std::endl;
			SDPCallback cb(output);			
			SDPClient rtpClient(env, &cb, sdp.c_str(), std::map<std::string,std::string>());
			
			signal(SIGINT, sig_handler);
			std::cout << "Start mainloop" << std::endl;
			env.mainloop();				
		}
	} 		
	else
	{
		usage(argv[0]);
	}
	return 0;
}
