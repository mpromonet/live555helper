# live555helper
Helper for live555

The aim of this project is to simplify usage of [live555 Streaming Media](http://www.live555.com/liveMedia/), that is powerful but need to implement lots of logic.

A simple RTSP client could be implement with :

	#include <iostream>
	#include "environment.h"
	#include "rtspconnectionclient.h"


	int main(int argc, char *argv[])
	{
		class MyCallback : public RTSPConnection::Callback
		{
			public:
				virtual bool    onData(const char* id, unsigned char* buffer, ssize_t size, struct timeval presentationTime) {
					std::cout << id << " " << size << " ts:" << presentationTime.tv_sec << "." << presentationTime.tv_usec << std::endl;
					return true;
				}
		};

		Environment env;
		MyCallback cb;
		RTSPConnection rtspClient(env, &cb, "rtsp://pi2.local:8554/unicast");
		env.mainloop();
		return 0;
	}
	
A simple SDP client could be implement with :

	#include <iostream>
	#include "environment.h"
	#include "sdpclient.h"


	int main(int argc, char *argv[])
	{
		class MyCallback : public SDPClient::Callback
		{
			public:
				virtual bool    onData(const char* id, unsigned char* buffer, ssize_t size, struct timeval presentationTime) {
					std::cout << id << " " << size << " ts:" << presentationTime.tv_sec << "." << presentationTime.tv_usec << std::endl;
					return true;
				}
		};

		std::string ip("127.0.0.1");
		std::string port("6901");
		std::string rtppayloadtype("96");
		std::string codec("H264");

		std::ostringstream os;
		os << "m=video " << port << " RTP/AVP " << rtppayloadtype << "\r\n"
		   << "c=IN IP4 " << ip <<"\r\n"
		   << "a=rtpmap:" << rtppayloadtype << " " << codec << "\r\n";
			
		Environment env;
		MyCallback cb;
		SDPClient rtpClient(env, &cb, os.str().c_str());
		env.mainloop();
		return 0;
	}

A simple MKV client could be implement with :

	#include <iostream>
	#include "environment.h"
	#include "mkvclient.h"

	int main(int argc, char *argv[])
	{
		class MyCallback : public MKVClient::Callback
		{
			public:
				virtual bool    onData(const char* id, unsigned char* buffer, ssize_t size, struct timeval presentationTime) {
					std::cout << id << " " << size << " ts:" << presentationTime.tv_sec << "." << presentationTime.tv_usec << std::endl;
					return true;
				}
		};

		Environment env;
		MyCallback cb;
		MKVClient mkvClient(env, &cb, "file://2008-Big_Buck_Bunny-496k.mkv");
		env.mainloop();
		return 0;
	}
