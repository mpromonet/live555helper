/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** SessionSink.cpp
** 
** -------------------------------------------------------------------------*/


#include "SessionSink.h"

SessionSink::SessionSink(UsageEnvironment& env, SessionCallback* callback, size_t bufferSize) 
	: MediaSink(env)
	, m_buffer(NULL)
	, m_bufferSize(bufferSize)
	, m_callback(callback) 
	, m_markerSize(0)
{
}

SessionSink::~SessionSink()
{
	m_callback->onCloseSession(this->name());
	delete [] m_buffer;
}

void SessionSink::allocate(ssize_t bufferSize)
{
	m_bufferSize = bufferSize;
	m_buffer = new u_int8_t[m_bufferSize];
	if (m_callback)
	{
		m_markerSize = m_callback->onNewBuffer(this->name(), this->source()->MIMEtype(), m_buffer, m_bufferSize);
	}
}


void SessionSink::afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime, unsigned durationInMicroseconds)
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

Boolean SessionSink::continuePlaying()
{
	if (m_buffer == NULL) 
	{
		allocate(m_bufferSize);
	}
	Boolean ret = False;
	if ( (m_buffer != NULL) && (source() != NULL) )
	{
		source()->getNextFrame(m_buffer+m_markerSize, m_bufferSize-m_markerSize,
				afterGettingFrame, this,
				onSourceClosure, this);
		ret = True;
	}
	return ret;	
}


