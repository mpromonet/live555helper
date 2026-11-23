/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** mkvclient.h
** 
** Interface to an MKV client
** 
** -------------------------------------------------------------------------*/

#pragma once

#include <atomic>
#include "environment.h"
#include "SessionSink.h"
#include "liveMedia.hh"
#include <string>
#include <map>
#include <vector>


/* ---------------------------------------------------------------------------
**  MKV client connection interface
** -------------------------------------------------------------------------*/
class MKVClient 
{
	public:
		/* ---------------------------------------------------------------------------
		**  MKV client callback interface
		** -------------------------------------------------------------------------*/
		class Callback : public SessionCallback
		{
			public:
				virtual void    onError(MKVClient&, const char*)  {}
				virtual void    onConnectionTimeout(MKVClient&)   {}
				virtual void    onDataTimeout(MKVClient&)         {}				
				virtual void    onEndOfFile(MKVClient&)  {}
		};
			
	public:
		MKVClient(Environment& env, Callback* callback, const char* path, const std::map<std::string,std::string> & opts, int verbosityLevel=1);
		virtual ~MKVClient();

		void start() {};
		void stop();
	
	private:
		void onMatroskaFileCreation(MatroskaFile* newFile);
		static void onMatroskaFileCreation(MatroskaFile* newFile, void* clientData) {
			((MKVClient*)(clientData))->onMatroskaFileCreation(newFile);
		}
		void onEndOfFile();
		static void onEndOfFile(void* clientData) {
			((MKVClient*)(clientData))->onEndOfFile();
		}
		void restart();
		static void restart(void* clientData) {
			((MKVClient*)(clientData))->restart();
		}
		static void onDemuxDeletion(void* clientData, MatroskaDemux*) {
			static_cast<MKVClient*>(clientData)->m_demux = nullptr;
		}
		void closeFile();
		
	protected:
		Environment&             m_env;
		std::string              m_url;
		Callback*                m_callback; 	
		MatroskaFile*            m_mkvfile = nullptr;
		MatroskaDemux*	         m_demux = nullptr;
		std::vector<MediaSink*>  m_sinks;
		std::vector<FramedSource*> m_tracks;
		SessionSink::PlaybackClock m_playbackClock;
		unsigned                 m_pendingEOF = 0;
		std::atomic<bool>        m_stopped = false;
		TaskToken                m_restartTask = nullptr;
};
