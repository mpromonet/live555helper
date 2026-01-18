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
#include <cxxopts.hpp>
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
	std::cout << "Usage: " << app << " [options] <url>" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -h               Show this help message and exit" << std::endl;
    std::cout << "  -v <level>       Set verbosity level (default: 255)" << std::endl;
    std::cout << "  -t <timeout>     Set timeout in seconds (default: 10)" << std::endl;
    std::cout << "  -o <output>      Set output file prefix for saving media data" << std::endl;
    std::cout << "  -M               Use multicast RTP transport" << std::endl;
    std::cout << "  -T               Use TCP RTP transport" << std::endl;
    std::cout << "  -H               Use HTTP RTP transport" << std::endl;
    std::cout << "Arguments:" << std::endl;
    std::cout << "  <url>            RTSP, RTSPS, file, or RTP URL to stream or process" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  " << app << " -v 1 -t 20 -o output rtsp://example.com/stream" << std::endl;
    std::cout << "  " << app << " -M file://example.mkv" << std::endl;

}

int main(int argc, char *argv[])
{
	// default value
	int  timeout = 10;
	std::string rtptransport = "udp";
	int  logLevel = 255;
	std::string output;
	std::map<std::string,std::string> opts;

	// decode args with cxxopts
	try {
		cxxopts::Options options(argv[0], "Simple Test Using live555helper");
		options.positional_help("<url>");
		options.add_options()
			("h,help", "Show help")
			("v,verbose", "Set verbosity level", cxxopts::value<int>(logLevel)->default_value("255"))
			("t,timeout", "Timeout in seconds", cxxopts::value<int>(timeout)->default_value("10"))
			("o,output", "Output file prefix", cxxopts::value<std::string>(output)->default_value(""))
			("M", "Use multicast RTP transport", cxxopts::value<bool>()->default_value("false"))
			("T", "Use TCP RTP transport", cxxopts::value<bool>()->default_value("false"))
			("H", "Use HTTP RTP transport", cxxopts::value<bool>()->default_value("false"))
			("url", "Input URL", cxxopts::value<std::vector<std::string>>());

		options.parse_positional({"url"});
		auto result = options.parse(argc, argv);

		if (result.count("help")) {
			usage(argv[0]);
			std::cout << options.help() << std::endl;
			return 0;
		}

		// RTP transport selection (priority: M > T > H if multiple provided)
		if (result.count("M") && result["M"].as<bool>()) {
			rtptransport = "multicast";
		} else if (result.count("T") && result["T"].as<bool>()) {
			rtptransport = "tcp";
		} else if (result.count("H") && result["H"].as<bool>()) {
			rtptransport = "http";
		}

		// Ensure we have exactly one positional URL
		if (!result.count("url") || result["url"].as<std::vector<std::string>>().empty()) {
			usage(argv[0]);
			std::cout << options.help() << std::endl;
			return 0;
		}

		// Replace argv/optind usage with parsed URL
		std::string url = result["url"].as<std::vector<std::string>>().front();

		opts["timeout"] = std::to_string(timeout);
		opts["rtptransport"] = rtptransport;

		Environment env(stop);
		if ( (url.find("rtsp://") == 0) || (url.find("rtsps://") == 0) ) {
			RTSPCallback cb(output);
			RTSPConnection rtspClient(env, &cb, url.c_str(), opts, logLevel);
			rtspClient.start();
            
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
		return 0;
	} catch (const std::exception& e) {
		std::cerr << "Argument parsing error: " << e.what() << std::endl;
		usage(argv[0]);
		return 1;
	}
	return 0;
}
