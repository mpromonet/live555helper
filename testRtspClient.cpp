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
#include "logger.h"
#include "environment.h"
#include "rtspconnectionclient.h"

class MyCallback : public RTSPConnection::Callback
{
	protected:
		Environment& m_env;
	
	public:
		MyCallback(Environment& env) : m_env(env) {
		}		
		virtual bool    onNewSession(const char* id, const char* media, const char* codec, const char* sdp) {
			std::cout << id << " " << media << "/" <<  codec << std::endl;
			return true;
		}
		virtual bool    onData(const char* id, unsigned char* buffer, ssize_t size, struct timeval presentationTime) {
			std::cout << id << " " << size << " ts:" << presentationTime.tv_sec << "." << presentationTime.tv_usec << std::endl;
			return true;
		}
		virtual ssize_t onNewBuffer(unsigned char* , ssize_t ) { 
			return 0; 
		}
		virtual void    onError(const char* message) {
			std::cout << "Error:" << message << std::endl;
			m_env.stop();
		}
		virtual void    onConnectionTimeout() {
			std::cout << "Connection timeout" << std::endl;
			m_env.stop();
		}
		virtual void    onDataTimeout()       {
			std::cout << "Data timeout" << std::endl;
			m_env.stop();
		}		
};

int main(int argc, char *argv[])
{
	if (argc > 0) 
	{
		initLogger(255);
		Environment env;
		MyCallback cb(env);
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
