# live555helper
Helper for live555

The aim of this project is to simplify usage of live555, that is powerful but need to implement lots of logic.

For instance a dummy RTSP client could be implement with :

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

                char stop = 0;
		Environment env;
		MyCallback cb;
		RTSPConnection rtspClient(env, &cb, "rtsp://pi2.local:8554/unicast");
		env.mainloop();
		return 0;
	}
	

