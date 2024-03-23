/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** rtspconnectionclient.h
** 
** Interface to an RTSP client connection
** 
** -------------------------------------------------------------------------*/

#pragma once

#include "environment.h"
#include "SessionSink.h"
#include "liveMedia.hh"
#include <string>
#include <map>

#define RTSP_CALLBACK(uri, resultCode, resultString) \
static void continueAfter ## uri(RTSPClient* rtspClient, int resultCode, char* resultString) { static_cast<RTSPConnection::RTSPClientConnection*>(rtspClient)->continueAfter ## uri(resultCode, resultString); } \
void continueAfter ## uri (int resultCode, char* resultString); \
/**/

#define TASK_CALLBACK(class,task) \
TaskToken m_ ## task ## Task; \
static void Task ## task(void* rtspClient) { static_cast<class*>(rtspClient)->Task ## task(); } \
void Task ## task (); \
/**/


#if LIVEMEDIA_LIBRARY_VERSION_INT > 1371168000 
	#define RTSPClientConstrutor(env, url, verbosity, appname, httpTunnelPort) RTSPClient(env, url, verbosity, appname, httpTunnelPort ,-1)
#else					
	#define RTSPClientConstrutor(env, url, verbosity, appname, httpTunnelPort) RTSPClient(env, url, verbosity, appname, httpTunnelPort)
#endif

/* ---------------------------------------------------------------------------
**  RTSP client connection interface
** -------------------------------------------------------------------------*/
class RTSPConnection 
{
	public:
		enum {
			RTPUDPUNICAST,
			RTPUDPMULTICAST,
			RTPOVERTCP,
			RTPOVERHTTP
		};
	
		static int decodeTimeoutOption(const std::map<std::string,std::string> & opts) {
			int timeout = 10;
			if (opts.find("timeout") != opts.end()) 
			{
				std::string timeoutString = opts.at("timeout");
				timeout = std::stoi(timeoutString);
			}
			return timeout;
		}

		static int decodeRTPTransport(const std::map<std::string,std::string> & opts) 
		{
			int rtptransport = RTSPConnection::RTPUDPUNICAST;
			if (opts.find("rtptransport") != opts.end()) 
			{
				std::string rtpTransportString = opts.at("rtptransport");
				if (rtpTransportString == "tcp") {
					rtptransport = RTSPConnection::RTPOVERTCP;
				} else if (rtpTransportString == "http") {
					rtptransport = RTSPConnection::RTPOVERHTTP;
				} else if (rtpTransportString == "multicast") {
					rtptransport = RTSPConnection::RTPUDPMULTICAST;
				}
			}
			return rtptransport;
		}	

		/* ---------------------------------------------------------------------------
		**  RTSP client callback interface
		** -------------------------------------------------------------------------*/
		class Callback : public SessionCallback
		{
			public:
				virtual void    onError(RTSPConnection&, const char*)  {}
				virtual void    onConnectionTimeout(RTSPConnection&)   {}
				virtual void    onDataTimeout(RTSPConnection&)         {}
		};
	
		/* ---------------------------------------------------------------------------
		**  RTSP client 
		** -------------------------------------------------------------------------*/
		class RTSPClientConnection : public RTSPClient
		{
			public:
				RTSPClientConnection(RTSPConnection& connection, Environment& env, Callback* callback, const char* rtspURL, int timeout, int rtptransport, int verbosityLevel = 0);
				virtual ~RTSPClientConnection(); 
			
			protected:
				void sendNextCommand(); 
				void setNptstartTime();

				RTSP_CALLBACK(DESCRIBE, resultCode, resultString);
				RTSP_CALLBACK(SETUP, resultCode, resultString);
				RTSP_CALLBACK(PLAY, resultCode, resultString);
				RTSP_CALLBACK(PAUSE, resultCode, resultString);
				RTSP_CALLBACK(TEARDOWN, resultCode, resultString);
			
				TASK_CALLBACK(RTSPConnection::RTSPClientConnection,ConnectionTimeout);
				TASK_CALLBACK(RTSPConnection::RTSPClientConnection,DataArrivalTimeout);
				
			protected:
				RTSPConnection&          m_connection;
				int                      m_timeout;
				int                      m_rtptransport;
				MediaSession*      		 m_session;                   
				MediaSubsession*         m_subSession;             
				MediaSubsessionIterator* m_subSessionIter;
				Callback*                m_callback; 	
				unsigned int             m_nbPacket;
				int						 m_playforinit;
				double					 m_nptStartTime;
				std::string				 m_clockStartTime;

		};
		
	public:
		RTSPConnection(Environment& env, Callback* callback, const char* rtspURL, const std::map<std::string,std::string> & opts, int verbosityLevel = 1);
		RTSPConnection(Environment& env, Callback* callback, const char* rtspURL, int timeout = 5, int rtptransport = RTPUDPUNICAST, int verbosityLevel = 1);
		virtual ~RTSPConnection();

		void        start(unsigned int delay = 0);
		void		stop();
		std::string getUrl() const          { return m_url; }
		int         getRtpTransport() const { return m_rtptransport; }

	protected:
		TASK_CALLBACK(RTSPConnection,startCallback);
		TASK_CALLBACK(RTSPConnection,stopCallback);
	
	protected:
		Environment&             m_env;
		Callback*                m_callback; 	
		std::string              m_url;
		int                      m_timeout;
		int                      m_rtptransport;
		int                      m_verbosity;
	
		RTSPClientConnection*    m_rtspClient;
};
