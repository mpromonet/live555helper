/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** mkvclient.cpp
** 
** Interface to an MKV client
** 
** -------------------------------------------------------------------------*/


#include <sstream>

#include "mkvclient.h"
#include "Base64.hh"

void MKVClient::onMatroskaFileCreation(MatroskaFile* newFile) {
	
	m_mkvfile = newFile;
	m_demux = m_mkvfile->newDemux();

	unsigned trackNumber = 0;
	FramedSource* trackSource = NULL;
	while ( (trackSource = m_demux->newDemuxedTrack(trackNumber)) != NULL) {
		m_env << "track:" << trackNumber << "\n";
		
		
		MatroskaTrack* track = m_mkvfile->lookup(trackNumber);
		if (track) {
			std::istringstream is(track->mimeType);
			std::string media;
			getline(is, media, '/');
			std::string codec;
			getline(is, codec, '/');
			
			char* encoded = base64Encode((const char*)track->codecPrivate, track->codecPrivateSize);

			MediaSink* sink = SessionSink::createNew(m_env, m_callback);	 
			if (sink == NULL) 
			{
				m_env << "Failed to create sink for \"" << track->mimeType << "\" subsession error: " << m_env.getResultMsg() << "\n";
				m_callback->onError(*this, m_env.getResultMsg());			
			} 
			else if (m_callback->onNewSession(sink->name(), media.c_str(), codec.c_str(), encoded)) 
			{
				m_env << "Start playing sink for \"" << track->mimeType << "\" codecPrivate:" << encoded << "\n";
				sink->startPlaying(*trackSource, NULL, NULL);	  
			} 
			else 
			{
				Medium::close(sink);
			}		

			delete [] encoded;
		}
	}
}


MKVClient::MKVClient(Environment& env, Callback* callback, const char* url) 
				: m_env(env)
				, m_callback(callback)
{
	const char* prefix = "file://";

	std::string fileurl(url);
	if (fileurl.find(prefix) == 0) {
		fileurl = fileurl.erase(0,strlen(prefix));
	}

	MatroskaFile::createNew(env, fileurl.c_str(), onMatroskaFileCreation, this);	
}

MKVClient::~MKVClient()
{	
	if (m_demux != NULL)  {
		Medium::close(m_demux);
	}
	if (m_mkvfile != NULL)  {
		Medium::close(m_mkvfile);
	}
}
		
