#include <iostream>
#include "logger.h"
#include "environment.h"
#include "rtspconnectionclient.h"

class MyCallback : public RTSPConnection::Callback
{
	public:
		virtual bool    onNewSession(const char* id, const char* media, const char* codec, const char* sdp) {
			std::cout << id << " " << media << "/" <<  codec << std::endl;
			return true;
		}
		virtual bool    onData(const char* id, unsigned char* buffer, ssize_t size) {
			std::cout << id << " " << size << std::endl;
			return true;
		}
		virtual ssize_t onNewBuffer(unsigned char* , ssize_t ) { return 0; };
};

int main(int argc, char *argv[])
{
	if (argc > 0) 
	{
		initLogger(255);
		Environment env;
		MyCallback cb;
		RTSPConnection rtspClient(env, &cb, argv[1]);
		std::cout << "Start mainloop" << std::endl;
		env.mainloop();	
	}
	else
	{
		std::cout << "Usage: " << argv[0] << " url" << std::endl;
	}
	return 0;
}