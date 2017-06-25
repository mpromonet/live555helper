/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** rtspconnectionclient.cpp
** 
** Interface to an RTSP client connection
** 
** -------------------------------------------------------------------------*/


#include "rtspconnectionclient.h"

RTSPConnection::SessionSink::SessionSink(UsageEnvironment& env, Callback* callback) 
	: MediaSink(env)
	, m_buffer(NULL)
	, m_bufferSize(0)
	, m_callback(callback) 
	, m_markerSize(0)
{
	allocate(1024*1024);
}

RTSPConnection::SessionSink::~SessionSink()
{
	delete [] m_buffer;
}

void RTSPConnection::SessionSink::allocate(ssize_t bufferSize)
{
	m_bufferSize = bufferSize;
	m_buffer = new u_int8_t[m_bufferSize];
	if (m_callback)
	{
		m_markerSize = m_callback->onNewBuffer(m_buffer, m_bufferSize);
		envir() << "markerSize:" << (int)m_markerSize << "\n";
	}
}


void RTSPConnection::SessionSink::afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime, unsigned durationInMicroseconds)
{
	if (numTruncatedBytes != 0)
	{
		delete [] m_buffer;
		envir() << "buffer too small " << (int)m_bufferSize << " allocate bigger one\n";
		allocate(m_bufferSize*2);
	}
	else if (m_callback)
	{
		if (!m_callback->onData(this->name(), m_buffer, frameSize+m_markerSize, presentationTime))
		{
			envir() << "NOTIFY failed\n";
		}
	}
	this->continuePlaying();
}

Boolean RTSPConnection::SessionSink::continuePlaying()
{
	Boolean ret = False;
	if (source() != NULL)
	{
		source()->getNextFrame(m_buffer+m_markerSize, m_bufferSize-m_markerSize,
				afterGettingFrame, this,
				onSourceClosure, this);
		ret = True;
	}
	return ret;	
}


RTSPConnection::RTSPConnection(Environment& env, Callback* callback, const char* rtspURL, int timeout, bool rtpovertcp, int verbosityLevel) 
				: m_startCallbackTask(NULL)
				, m_env(env)
				, m_callback(callback)
				, m_url(rtspURL)
				, m_timeout(timeout)
				, m_rtpovertcp(rtpovertcp)
				, m_verbosity(verbosityLevel)
				, m_rtspClient(NULL)
{
	this->start();
}

void RTSPConnection::start(unsigned int delay)
{
	m_startCallbackTask = m_env.taskScheduler().scheduleDelayedTask(delay*1000000, TaskstartCallback, this);
}	

void RTSPConnection::TaskstartCallback() 
{
	if (m_rtspClient)
	{
		
		Medium::close(m_rtspClient);
	}
	
	m_rtspClient = new RTSPClientConnection(*this, m_env, m_callback, m_url.c_str(), m_timeout, m_rtpovertcp, m_verbosity);	
}

RTSPConnection::~RTSPConnection()
{
	m_env.taskScheduler().unscheduleDelayedTask(m_startCallbackTask);
	Medium::close(m_rtspClient);
}

		
RTSPConnection::RTSPClientConnection::RTSPClientConnection(RTSPConnection& connection, Environment& env, Callback* callback, const char* rtspURL, int timeout, bool rtpovertcp, int verbosityLevel) 
				: RTSPClientConstrutor(env, rtspURL, verbosityLevel, NULL, 0)
				, m_connection(connection)
				, m_timeout(timeout)
				, m_rtpovertcp(rtpovertcp)
				, m_session(NULL)
				, m_subSessionIter(NULL)
				, m_callback(callback)
				, m_nbPacket(0)
{
	// start tasks
	m_ConnectionTimeoutTask = envir().taskScheduler().scheduleDelayedTask(m_timeout*1000000, TaskConnectionTimeout, this);
	
	// initiate connection process
	this->sendNextCommand();
}

RTSPConnection::RTSPClientConnection::~RTSPClientConnection()
{
	envir().taskScheduler().unscheduleDelayedTask(m_ConnectionTimeoutTask);
	envir().taskScheduler().unscheduleDelayedTask(m_DataArrivalTimeoutTask);
	
	delete m_subSessionIter;
	
	// free subsession
	if (m_session != NULL) 
	{
		MediaSubsessionIterator iter(*m_session);
		MediaSubsession* subsession;
		while ((subsession = iter.next()) != NULL) 
		{
			if (subsession->sink) 
			{
				envir() << "Close session: " << subsession->mediumName() << "/" << subsession->codecName() << "\n";
				Medium::close(subsession->sink);
				subsession->sink = NULL;
			}
		}	
		Medium::close(m_session);
	}
}
		
void RTSPConnection::RTSPClientConnection::sendNextCommand() 
{
	if (m_subSessionIter == NULL)
	{
		// no SDP, send DESCRIBE
		this->sendDescribeCommand(continueAfterDESCRIBE); 
	}
	else
	{
		m_subSession = m_subSessionIter->next();
		if (m_subSession != NULL) 
		{
			// still subsession to SETUP
			if (!m_subSession->initiate()) 
			{
				envir() << "Failed to initiate " << m_subSession->mediumName() << "/" << m_subSession->codecName() << " subsession: " << envir().getResultMsg() << "\n";
				this->sendNextCommand();
			} 
			else 
			{
				if (fVerbosityLevel > 1) 
				{				
					envir() << "Initiated " << m_subSession->mediumName() << "/" << m_subSession->codecName() << " subsession" << "\n";
				}
			}

			this->sendSetupCommand(*m_subSession, continueAfterSETUP, false, m_rtpovertcp);
		}
		else
		{
			// no more subsession to SETUP, send PLAY
			this->sendPlayCommand(*m_session, continueAfterPLAY);
		}
	}
}

void RTSPConnection::RTSPClientConnection::continueAfterDESCRIBE(int resultCode, char* resultString)
{
	if (resultCode != 0) 
	{
		envir() << "Failed to DESCRIBE: " << resultString << "\n";
		m_callback->onError(m_connection, resultString);
	}
	else
	{
		if (fVerbosityLevel > 1) 
		{
			envir() << "Got SDP:\n" << resultString << "\n";
		}
		m_session = MediaSession::createNew(envir(), resultString);
		m_subSessionIter = new MediaSubsessionIterator(*m_session);
		this->sendNextCommand();  
	}
	delete[] resultString;
}

void RTSPConnection::RTSPClientConnection::continueAfterSETUP(int resultCode, char* resultString)
{
	if (resultCode != 0) 
	{
		envir() << "Failed to SETUP: " << resultString << "\n";
		m_callback->onError(m_connection, resultString);
	}
	else
	{				
		if (m_callback->onNewSession(m_subSession->sink->name(), m_subSession->mediumName(), m_subSession->codecName(), m_subSession->savedSDPLines()))
		{
			envir() << "Created a data sink for the \"" << m_subSession->mediumName() << "/" << m_subSession->codecName() << "\" subsession" << "\n";
			m_subSession->sink = SessionSink::createNew(envir(), m_callback);
			if (m_subSession->sink == NULL) 
			{
				envir() << "Failed to create a data sink for " << m_subSession->mediumName() << "/" << m_subSession->codecName() << " subsession: " << envir().getResultMsg() << "\n";
			} else {
				m_subSession->sink->startPlaying(*(m_subSession->readSource()), NULL, NULL);
			}
		}
	}
	delete[] resultString;
	this->sendNextCommand();  
}	

void RTSPConnection::RTSPClientConnection::continueAfterPLAY(int resultCode, char* resultString)
{
	if (resultCode != 0) 
	{
		envir() << "Failed to PLAY: " << resultString << "\n";
		m_callback->onError(m_connection, resultString);
	}
	else
	{
		if (fVerbosityLevel > 1) 
		{
			envir() << "PLAY OK" << "\n";
		}
		m_DataArrivalTimeoutTask = envir().taskScheduler().scheduleDelayedTask(m_timeout*1000000, TaskDataArrivalTimeout, this);

	}
	envir().taskScheduler().unscheduleDelayedTask(m_ConnectionTimeoutTask);
	delete[] resultString;
}

void RTSPConnection::RTSPClientConnection::TaskConnectionTimeout()
{
	m_callback->onConnectionTimeout(m_connection);
}
		
void RTSPConnection::RTSPClientConnection::TaskDataArrivalTimeout()
{
	unsigned int newTotNumPacketsReceived = 0;

	MediaSubsessionIterator iter(*m_session);
	MediaSubsession* subsession;
	while ((subsession = iter.next()) != NULL) 
	{
		RTPSource* src = subsession->rtpSource();
		if (src != NULL) 
		{
			newTotNumPacketsReceived += src->receptionStatsDB().totNumPacketsReceived();
		}
	}
	
	if (newTotNumPacketsReceived == m_nbPacket) 
	{
		m_callback->onDataTimeout(m_connection);
	} 
	else 
	{
		m_nbPacket = newTotNumPacketsReceived;
		m_DataArrivalTimeoutTask = envir().taskScheduler().scheduleDelayedTask(m_timeout*1000000, TaskDataArrivalTimeout, this);
	}	
}
