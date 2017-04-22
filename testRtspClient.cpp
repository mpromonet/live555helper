/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** testRtspClient.cpp
** 
** Simple Test Using live555helper
** -------------------------------------------------------------------------*/

#include <iostream>
#include <signal.h>
#include "environment.h"
#include "rtspconnectionclient.h"

class MyCallback : public RTSPConnection::Callback
{
	public:
		virtual bool    onNewSession(const char* id, const char* media, const char* codec, const char* sdp) {
			std::cout << id << " " << media << "/" <<  codec << std::endl;
			return true;
		}
		virtual bool    onData(const char* id, unsigned char* buffer, ssize_t size, struct timeval presentationTime) {
			std::cout << id << " " << size << " ts:" << presentationTime.tv_sec << "." << presentationTime.tv_usec << std::endl;
			return true;
		}
		virtual void    onError(const char* message) {
			std::cout << "Error:" << message << std::endl;
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

char stop = 0;
void sig_handler(int signo)
{
	if (signo == SIGINT) {
		printf("received SIGINT\n");
		stop = 1;
	}
}

int main(int argc, char *argv[])
{
	if (argc > 0) 
	{
		Environment env(stop);
		MyCallback cb;
		int  timeout = 10;
		bool rtpovertcp = true;
		int  logLevel = 255;
		RTSPConnection rtspClient(env, &cb, argv[1], timeout, rtpovertcp, logLevel);
		signal(SIGINT, sig_handler);
		std::cout << "Start mainloop" << std::endl;
		env.mainloop();	
	}
	else
	{
		std::cout << "Usage: " << argv[0] << " url" << std::endl;
	}
	return 0;
}
