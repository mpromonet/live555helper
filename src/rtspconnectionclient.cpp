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


#include "logger.h"

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
		LOG(NOTICE) << "markerSize:" << m_markerSize;
	}
}


void RTSPConnection::SessionSink::afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime, unsigned durationInMicroseconds)
{
	LOG(DEBUG) << "NOTIFY size:" << frameSize;
	if (numTruncatedBytes != 0)
	{
		delete [] m_buffer;
		LOG(NOTICE) << "buffer too small " << m_bufferSize << " allocate bigger one\n";
		allocate(m_bufferSize*2);
	}
	else if (m_callback)
	{
		if (!m_callback->onData(this->name(), m_buffer, frameSize+m_markerSize))
		{
			LOG(WARN) << "NOTIFY failed";
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


		
RTSPConnection::RTSPConnection(Environment& env, Callback* callback, const char* rtspURL, int timeout, int verbosityLevel) 
				: RTSPClient(env, rtspURL, verbosityLevel, NULL, 0
#if LIVEMEDIA_LIBRARY_VERSION_INT > 1371168000 
					,-1
#endif
					)
				, m_timeout(timeout)
				, m_session(NULL)
				, m_subSessionIter(NULL)
				, m_callback(callback)
				, m_env(env)
				, m_nbPacket(0)
{
	// start tasks
	m_connectionTask = env.taskScheduler().scheduleDelayedTask(timeout*1000000, TaskConnectionTimeout, this);
	m_dataTask       = env.taskScheduler().scheduleDelayedTask(timeout*1000000, TaskDataArrivalTimeout, this);
	
	// initiate connection process
	this->sendNextCommand();
}

RTSPConnection::~RTSPConnection()
{
	delete m_subSessionIter;
	Medium::close(m_session);
}
		
void RTSPConnection::sendNextCommand() 
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
				LOG(WARN) << "Failed to initiate " << m_subSession->mediumName() << "/" << m_subSession->codecName() << " subsession: " << envir().getResultMsg();
				this->sendNextCommand();
			} 
			else 
			{					
				LOG(NOTICE) << "Initiated " << m_subSession->mediumName() << "/" << m_subSession->codecName() << " subsession";
			}

			this->sendSetupCommand(*m_subSession, continueAfterSETUP);
		}
		else
		{
			// no more subsession to SETUP, send PLAY
			this->sendPlayCommand(*m_session, continueAfterPLAY);
		}
	}
}

void RTSPConnection::continueAfterDESCRIBE(int resultCode, char* resultString)
{
	if (resultCode != 0) 
	{
		LOG(WARN) << "Failed to DESCRIBE: " << resultString;
		m_env.stop();
	}
	else
	{
		LOG(NOTICE) << "Got SDP:\n" << resultString;
		m_session = MediaSession::createNew(envir(), resultString);
		m_subSessionIter = new MediaSubsessionIterator(*m_session);
		this->sendNextCommand();  
	}
	delete[] resultString;
}

void RTSPConnection::continueAfterSETUP(int resultCode, char* resultString)
{
	if (resultCode != 0) 
	{
		LOG(WARN) << "Failed to SETUP: " << resultString;
		m_env.stop();
	}
	else
	{				
		m_subSession->sink = SessionSink::createNew(envir(), m_callback);
		if (m_subSession->sink == NULL) 
		{
			LOG(WARN) << "Failed to create a data sink for " << m_subSession->mediumName() << "/" << m_subSession->codecName() << " subsession: " << envir().getResultMsg() << "\n";
		}
		else if (m_callback->onNewSession(m_subSession->sink->name(), m_subSession->mediumName(), m_subSession->codecName(), m_subSession->savedSDPLines()))
		{
			LOG(WARN) << "Created a data sink for the \"" << m_subSession->mediumName() << "/" << m_subSession->codecName() << "\" subsession";
			m_subSession->sink->startPlaying(*(m_subSession->readSource()), NULL, NULL);
		}
	}
	delete[] resultString;
	this->sendNextCommand();  
}	

void RTSPConnection::continueAfterPLAY(int resultCode, char* resultString)
{
	if (resultCode != 0) 
	{
		LOG(WARN) << "Failed to PLAY: " << resultString;
		m_env.stop();
	}
	else
	{
		LOG(NOTICE) << "PLAY OK";
		envir().taskScheduler().unscheduleDelayedTask(m_connectionTask);
	}
	delete[] resultString;
}

void RTSPConnection::TaskConnectionTimeout()
{
	std::cout << "timeout" << std::endl;
	m_env.stop();
}
		
void RTSPConnection::TaskDataArrivalTimeout()
{
	unsigned int newTotNumPacketsReceived = 0;

	MediaSubsessionIterator iter(*m_session);
	MediaSubsession* subsession;
	while ((subsession = iter.next()) != NULL) {
		RTPSource* src = subsession->rtpSource();
		if (src != NULL) 
		{
			newTotNumPacketsReceived += src->receptionStatsDB().totNumPacketsReceived();
		}
	}

	if (newTotNumPacketsReceived == m_nbPacket) {
		std::cout << "no more data" << std::endl;
		m_env.stop();	  
	} else {
		m_nbPacket = newTotNumPacketsReceived;
		m_dataTask       = envir().taskScheduler().scheduleDelayedTask(m_timeout*1000000, TaskDataArrivalTimeout, this);
	}	
}
