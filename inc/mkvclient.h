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

#include "environment.h"
#include "SessionSink.h"
#include "liveMedia.hh"
#include <string>


/* ---------------------------------------------------------------------------
**  SDP client connection interface
** -------------------------------------------------------------------------*/
class MKVClient 
{
	public:
		/* ---------------------------------------------------------------------------
		**  SDP client callback interface
		** -------------------------------------------------------------------------*/
		class Callback : public SessionCallback
		{
			public:
				virtual void    onError(MKVClient&, const char*)  {}
		};
			
	public:
		MKVClient(Environment& env, Callback* callback, const char* path);
		virtual ~MKVClient();
	
	private:
		void onMatroskaFileCreation(MatroskaFile* newFile);
		static void onMatroskaFileCreation(MatroskaFile* newFile, void* clientData) {
			((MKVClient*)(clientData))->onMatroskaFileCreation(newFile);
		}
	
	protected:
		Environment&             m_env;
		Callback*                m_callback; 	
		MatroskaFile*            m_mkvfile; 
		MatroskaDemux*	         m_demux;
};
