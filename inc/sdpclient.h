/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** sdpclient.h
** 
** Interface to an SDP client
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
class SDPClient 
{
	public:
		/* ---------------------------------------------------------------------------
		**  SDP client callback interface
		** -------------------------------------------------------------------------*/
		class Callback : public SessionCallback
		{
			public:
				virtual void    onError(SDPClient&, const char*)  {}
		};
			
	public:
		SDPClient(Environment& env, Callback* callback, const char* SDP);
		virtual ~SDPClient();
	
	protected:
		Environment&             m_env;
		Callback*                m_callback; 	
		MediaSession*            m_session;                   	
};
