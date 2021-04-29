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
#include <algorithm>
#include <iostream>
#include <sstream>


RTSPConnection::RTSPConnection(Environment& env, Callback* callback, const char* rtspURL, int timeout, int rtptransport, int verbosityLevel) 
				: m_startCallbackTask(NULL)
				, m_env(env)
				, m_callback(callback)
				, m_url(rtspURL)
				, m_timeout(timeout)
				, m_rtptransport(rtptransport)
				, m_verbosity(verbosityLevel)
				, m_rtspClient(NULL)
{
	this->start();
}

RTSPConnection::RTSPConnection(Environment& env, Callback* callback, const char* rtspURL, const std::map<std::string,std::string> & opts, int verbosityLevel) 
				: m_startCallbackTask(NULL)
				, m_env(env)
				, m_callback(callback)
				, m_url(rtspURL)
				, m_timeout(decodeTimeoutOption(opts))
				, m_rtptransport(decodeRTPTransport(opts))
				, m_verbosity(verbosityLevel)
				, m_rtspClient(NULL)
{
	this->start();
}

void RTSPConnection::start(unsigned int delay)
{
	if (m_startCallbackTask) {
		m_env.taskScheduler().unscheduleDelayedTask(m_startCallbackTask);
	}
	m_startCallbackTask = m_env.taskScheduler().scheduleDelayedTask(delay*1000000, TaskstartCallback, this);
}	

void RTSPConnection::TaskstartCallback() 
{
	if (m_rtspClient)
	{
		
		Medium::close(m_rtspClient);
	}
	
	m_rtspClient = new RTSPClientConnection(*this, m_env, m_callback, m_url.c_str(), m_timeout, m_rtptransport, m_verbosity);	
}

RTSPConnection::~RTSPConnection()
{
	m_env.taskScheduler().unscheduleDelayedTask(m_startCallbackTask);
	Medium::close(m_rtspClient);
}

int getHttpTunnelPort(int  rtptransport, const char* rtspURL) 
{
	int httpTunnelPort = 0;
	if (rtptransport == RTSPConnection::RTPOVERHTTP) 
	{
		std::string url = rtspURL;
                const char * pattern = "://";
                std::size_t pos = url.find(pattern);
                if (pos != std::string::npos) {
                        url.erase(0,pos+strlen(pattern));
                }
                pos = url.find_first_of("/");
                if (pos != std::string::npos) {
                        url.erase(pos);
                }
                pos = url.find_first_of(":");
                if (pos != std::string::npos) {
			url.erase(0,pos+1);
                        httpTunnelPort = std::stoi(url);
                }
	}
	return httpTunnelPort;
}
		
RTSPConnection::RTSPClientConnection::RTSPClientConnection(RTSPConnection& connection, Environment& env, Callback* callback, const char* rtspURL, int timeout, int  rtptransport, int verbosityLevel) 
				: RTSPClientConstrutor(env, rtspURL, verbosityLevel, NULL, getHttpTunnelPort(rtptransport, rtspURL))
				, m_connection(connection)
				, m_timeout(timeout)
				, m_rtptransport(rtptransport)
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
		if (fVerbosityLevel > 1)
		{
			envir() << " Send Teardown command" << "\n";
		}
		this->sendTeardownCommand(*m_session, continueAfterTEARDOWN);
		Medium::close(m_session);
	}
}

/// <summary>
/// Get params npttime or starttime from a query string 
/// urlStr rtsp://192.168.31.124:554/camera02?npttime=74652
/// urlStr rtsp://192.168.31.124:554/camera02?starttime=20210129T103000Z
/// Be aware only the first params is sent to here
/// </summary>
void RTSPConnection::RTSPClientConnection::setNptstartTime()
{
	m_nptStartTime = 0;
	m_playforinit = 0;
	std::string urlStr = m_connection.getUrl();
	std::istringstream is(urlStr);
	std::map<std::string, std::string> opts;
	std::string key, value, querystring;
	while (std::getline(std::getline(std::getline(is, querystring, '?'), key, '='), value, '&'))
	{
		opts[key] = value;
	}

	if (opts.count("npttime") > 0)
	{
		m_nptStartTime = std::stod(opts.at("npttime"));
	}
	
	if (opts.find("starttime") != opts.end())
	{
		m_clockStartTime = opts["starttime"];
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
				this->sendSetupCommand(*m_subSession, continueAfterSETUP, false, (m_rtptransport == RTPOVERTCP), (m_rtptransport == RTPUDPMULTICAST) );
			}
		}
		else
		{
			// no more subsession to SETUP, send PLAY
			setNptstartTime();
			if (fVerbosityLevel > 1)
			{
				envir() << "****************************************************************************************\n";
				envir() << " nptTime is given read video from it :: m_nptStartTime " << m_nptStartTime << "\n";
				envir() << " clockstarttime is given read video from it :: m_clockStartTime " << m_clockStartTime.c_str() << "\n";
			}
			if (m_clockStartTime != "") {
				m_playforinit = 1;
				this->sendPlayCommand(*m_session, continueAfterPLAY, m_clockStartTime.c_str());
			}
			else if (m_nptStartTime > 0) {
				m_playforinit = 1;
				this->sendPlayCommand(*m_session, continueAfterPLAY, m_nptStartTime);
			}
			else {
				m_playforinit = 0;
				this->sendPlayCommand(*m_session, continueAfterPLAY);
			}
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
		if (m_session)
		{
			m_subSessionIter = new MediaSubsessionIterator(*m_session);
			this->sendNextCommand();
		}
		else
		{
			if (fVerbosityLevel > 1)
			{
				envir() << "MediaSession::createNew() failed! (result string: " << resultString << ")\n";
			}
			m_callback->onError(m_connection, "MediaSession::createNew() failed!");
		}
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
		envir() << " Requested URL : " << m_connection.getUrl().c_str() << "\n";
		MediaSink* sink = SessionSink::createNew(envir(), m_callback);
		if (sink == NULL) 
		{
			envir() << "Failed to create sink for \"" << m_subSession->mediumName() << "/" << m_subSession->codecName() << "\" subsession error: " << envir().getResultMsg() << "\n";
			m_callback->onError(m_connection, envir().getResultMsg());			
		} 
		else if (m_callback->onNewSession(sink->name(), m_subSession->mediumName(), m_subSession->codecName(), m_subSession->savedSDPLines())) 
		{
			envir() << "Start playing sink for \"" << m_subSession->mediumName() << "/" << m_subSession->codecName() << "\" subsession" << "\n";
			m_subSession->sink = sink;
			m_subSession->sink->startPlaying(*(m_subSession->readSource()), NULL, NULL);
		} 
		else 
		{
			Medium::close(sink);
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
		if (m_playforinit > 0)
		{
			envir() << "PLAY INIT OK" << "\n";
			this->sendPauseCommand(*m_session, continueAfterPAUSE);
		}
		else {
			if (fVerbosityLevel > 1) 
			{
				envir() << "PLAY OK" << "\n";
			}
			m_DataArrivalTimeoutTask = envir().taskScheduler().scheduleDelayedTask(m_timeout*1000000, TaskDataArrivalTimeout, this);
		}
	}
	envir().taskScheduler().unscheduleDelayedTask(m_ConnectionTimeoutTask);
	delete[] resultString;
}

void RTSPConnection::RTSPClientConnection::continueAfterPAUSE(int resultCode, char* resultString)
{
	if (resultCode != 0)
	{
		envir() << "Failed to PLAUSE: " << resultString << "\n";
		m_callback->onError(m_connection, resultString);
	}
	else if (m_playforinit > 0)
	{
		if (fVerbosityLevel > 1)
		{
			envir() << " continueAfterPAUSE m_playforinit: " << m_playforinit << "\n";
			envir() << " continueAfterPAUSE m_nptStartTime: " << m_nptStartTime << "\n";
			envir() << " continueAfterPAUSE m_clockStartTime " << m_clockStartTime.c_str() << "\n";
		}
		m_playforinit = 0;
		if (m_clockStartTime != "") {
			this->sendPlayCommand(*m_session, continueAfterPLAY, m_clockStartTime.c_str());
		}
		else if (m_nptStartTime > 0) {
			this->sendPlayCommand(*m_session, continueAfterPLAY, m_nptStartTime);
		}
		else {
			this->sendPlayCommand(*m_session, continueAfterPLAY);
		}
	}
	delete[] resultString;
}

void RTSPConnection::RTSPClientConnection::continueAfterTEARDOWN(int resultCode, char* resultString)
{
	if (resultCode != 0)
	{
		envir() << "Failed to TEARDOWN: " << resultString << "\n";
		m_callback->onError(m_connection, resultString);
	}
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
