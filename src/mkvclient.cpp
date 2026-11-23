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

void MKVClient::onMatroskaFileCreation(MatroskaFile* newFile) {
	
	m_mkvfile = newFile;
	m_demux = m_mkvfile->newDemux(onDemuxDeletion, this);

	unsigned trackNumber = 0;
	FramedSource* trackSource = NULL;
	while ( (trackSource = m_demux->newDemuxedTrack(trackNumber)) != NULL) {
		m_tracks.push_back(trackSource);
		m_env << "track:" << trackNumber << "\n";
		
		
		MatroskaTrack* track = m_mkvfile->lookup(trackNumber);
		if (track) {
			std::istringstream is(track->mimeType);
			std::string media;
			getline(is, media, '/');
			std::string codec;
			getline(is, codec, '/');
			
			std::ostringstream os;
			struct sockaddr_storage destinationAddress;
			const Port rtpPort(0);
			Groupsock gs(m_env, destinationAddress, rtpPort, 0);

			RTPSink* rtpsink = m_mkvfile->createRTPSinkForTrackNumber(trackNumber, &gs, 96);
			char* rtpmap = rtpsink->rtpmapLine();
			os << rtpmap;
			delete[] rtpmap;
			const char* auxLine = rtpsink->auxSDPLine();
			if (auxLine) {
				os << auxLine;

			}
			unsigned int frequency = rtpsink->rtpTimestampFrequency();
			unsigned int channels = rtpsink->numChannels();
			Medium::close(rtpsink);
			std::string sdp(os.str());

			MediaSink* sink = SessionSink::createNew(m_env, m_callback, 2*1024*1024, &m_playbackClock);
			if (sink == NULL) 
			{
				m_env << "Failed to create sink for \"" << track->mimeType << "\" subsession error: " << m_env.getResultMsg() << "\n";
				m_callback->onError(*this, m_env.getResultMsg());			
			} 
			else if (m_callback->onNewSession(sink->name(), media.c_str(), codec.c_str(), sdp.c_str(), frequency, channels))
			{
				m_env << "Start playing sink for \"" << track->mimeType << "\" sdp:" << sdp.c_str() << "\n";
				if (sink->startPlaying(*trackSource, onEndOfFile, this)) {
					m_sinks.push_back(sink);
					++m_pendingEOF;
				} else {
					Medium::close(sink);
				}
			} 
			else 
			{
				// MKV need to start all tracks to start to read the file
				Medium::close(sink);
				MediaSink* sink = SessionSink::createNew(m_env, NULL, 2*1024*1024, &m_playbackClock);
				if (sink != NULL) {
					if (sink->startPlaying(*trackSource, onEndOfFile, this)) {
						m_sinks.push_back(sink);
						++m_pendingEOF;
					} else {
						Medium::close(sink);
					}
				}
			}		
		}
	}
}

void MKVClient::onEndOfFile() {
	if (m_pendingEOF == 0 || --m_pendingEOF != 0) {
		return;
	}
	if (m_callback) {
		m_callback->onEndOfFile(*this);
	}
	if (!m_stopped.load()) {
		m_restartTask = m_env.taskScheduler().scheduleDelayedTask(m_playbackClock.remaining(), restart, this);
	}
}

void MKVClient::restart() {
	m_restartTask = nullptr;
	if (m_stopped.load()) {
		return;
	}
	closeFile();
	m_pendingEOF = 0;
	m_playbackClock = SessionSink::PlaybackClock();
	MatroskaFile::createNew(m_env, m_url.c_str(), onMatroskaFileCreation, this);
}

void MKVClient::closeFile() {
	for (FramedSource* track : m_tracks) {
		track->stopGettingFrames();
	}
	for (MediaSink* sink : m_sinks) {
		Medium::close(sink);
	}
	m_sinks.clear();
	// Closing the last track deletes the demux automatically.
	for (FramedSource* track : m_tracks) {
		Medium::close(track);
	}
	m_tracks.clear();
	if (m_demux != NULL) {
		Medium::close(m_demux);
		m_demux = NULL;
	}
	if (m_mkvfile != NULL) {
		Medium::close(m_mkvfile);
		m_mkvfile = NULL;
	}
}
	

MKVClient::MKVClient(Environment& env, Callback* callback, const char* url, const std::map<std::string,std::string> & opts, int verbosityLevel) 
				: m_env(env)
				, m_callback(callback)
{
	const char* prefix = "file://";

	m_url = url;
	if (m_url.find(prefix) == 0) {
		m_url.erase(0,strlen(prefix));
	}

	MatroskaFile::createNew(env, m_url.c_str(), onMatroskaFileCreation, this);
}

MKVClient::~MKVClient()
{
	m_stopped.store(true);
	if (m_restartTask != nullptr) {
		m_env.taskScheduler().unscheduleDelayedTask(m_restartTask);
		m_restartTask = nullptr;
	}
	closeFile();
}

void MKVClient::stop() {
	m_stopped.store(true);
	m_env.stop();
}
		
