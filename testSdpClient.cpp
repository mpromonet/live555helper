/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** testSdpClient.cpp
** 
** Simple Test Using live555helper
** -------------------------------------------------------------------------*/

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <signal.h>
#include "environment.h"
#include "sdpclient.h"

class MyCallback : public SDPClient::Callback
{
	private:
		std::map<std::string,std::ofstream> m_ofs;
		std::string m_fileprefix;
		
	public:
		MyCallback(const std::string & output) : m_fileprefix(output)  {}
		
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
	std::string output;
	
	// decode args
	int c = 0;
	while ((c = getopt (argc, argv, "hv:" "o:")) != -1)
	{
		switch (c)
		{
			case 'h':	usage(argv[0]);  return 0;
			
			case 'o':	output = optarg;  break;			
		}
	}
	
	if (optind<argc)
	{
		Environment env(stop);
		MyCallback cb(output);
		std::string url = argv[optind];
		
		std::string sdp;
		if (url.find("rtp://") == 0) {
			std::istringstream is(url.substr(strlen("rtp://")));
			std::string ip;
			std::getline(is, ip, ':');
			std::string port;
			std::getline(is, port, '/');
			std::string rtppayloadtype("96");
			std::getline(is, rtppayloadtype, '/');
			std::string codec("H264");
			std::getline(is, codec);
			
			std::ostringstream os;
			os << "m=video " << port << " RTP/AVP " << rtppayloadtype << "\r\n"
			   << "c=IN IP4 " << ip <<"\r\n"
			   << "a=rtpmap:" << rtppayloadtype << " " << codec << "\r\n";
			sdp.assign(os.str());

		} else {		
			std::ifstream is(url);
			sdp.assign((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());
		}
		std::cout << "SDP:\n" << sdp << std::endl;
		
		SDPClient rtpClient(env, &cb, sdp.c_str());
		signal(SIGINT, sig_handler);
		std::cout << "Start mainloop" << std::endl;
		env.mainloop();	
	} 		
	else
	{
		usage(argv[0]);
	}
	return 0;
}
