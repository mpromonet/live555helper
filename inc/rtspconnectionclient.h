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
#include "liveMedia.hh"


#define RTSP_CALLBACK(uri, resultCode, resultString) \
static void continueAfter ## uri(RTSPClient* rtspClient, int resultCode, char* resultString) { static_cast<RTSPConnection*>(rtspClient)->continueAfter ## uri(resultCode, resultString); } \
void continueAfter ## uri (int resultCode, char* resultString); \
/**/


/* ---------------------------------------------------------------------------
**  RTSP client connection interface
** -------------------------------------------------------------------------*/
class RTSPConnection : public RTSPClient
{
	public:
		/* ---------------------------------------------------------------------------
		**  RTSP client callback interface
		** -------------------------------------------------------------------------*/
		class Callback
		{
			public:
				virtual bool    onNewSession(const char* id, const char* media, const char* codec, const char* sdp) = 0;
				virtual bool    onData(const char* id, unsigned char* buffer, ssize_t size) = 0;
				virtual ssize_t onNewBuffer(unsigned char* , ssize_t ) { return 0; };
		};

	protected:
		/* ---------------------------------------------------------------------------
		**  RTSP client Sink
		** -------------------------------------------------------------------------*/
		class SessionSink: public MediaSink 
		{
			public:
				static SessionSink* createNew(UsageEnvironment& env, Callback* callback) { return new SessionSink(env, callback); }

			private:
				SessionSink(UsageEnvironment& env, Callback* callback);
				virtual ~SessionSink();

				void allocate(ssize_t bufferSize);

				static void afterGettingFrame(void* clientData, unsigned frameSize,
							unsigned numTruncatedBytes,
							struct timeval presentationTime,
							unsigned durationInMicroseconds)
				{
					static_cast<SessionSink*>(clientData)->afterGettingFrame(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
				}
				
				void afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime, unsigned durationInMicroseconds);

				virtual Boolean continuePlaying();

			private:
				u_int8_t*              m_buffer;
				size_t                 m_bufferSize;
				Callback*              m_callback; 	
				ssize_t                m_markerSize;
		};
	
	public:
		RTSPConnection(Environment& env, Callback* callback, const char* rtspURL, int timeout = 5, int verbosityLevel = 255);
		virtual ~RTSPConnection();

	protected:
		void sendNextCommand(); 
				
		RTSP_CALLBACK(DESCRIBE,resultCode,resultString);
		RTSP_CALLBACK(SETUP,resultCode,resultString);
		RTSP_CALLBACK(PLAY,resultCode,resultString);
	
		static void checkConnectionTimeout(void* rtspClient) { static_cast<RTSPConnection*>(rtspClient)->checkConnectionTimeout(); };
		void checkConnectionTimeout();
		
	protected:
		MediaSession*            m_session;                   
		MediaSubsession*         m_subSession;             
		MediaSubsessionIterator* m_subSessionIter;
		Callback*                m_callback; 	
		Environment&             m_env;
		TaskToken 		 m_connectionTask;
};
