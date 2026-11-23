/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** SessionSink.cpp
** 
** -------------------------------------------------------------------------*/


#include "SessionSink.h"

#include <algorithm>

int64_t SessionSink::PlaybackClock::delay(struct timeval presentationTime, unsigned durationInMicroseconds)
{
	const int64_t timestamp = static_cast<int64_t>(presentationTime.tv_sec) * 1000000 + presentationTime.tv_usec;
	const auto now = std::chrono::steady_clock::now();
	if (!m_started) {
		m_started = true;
		m_firstPresentationTime = timestamp;
		m_start = now;
		m_end = now;
	}
	const auto target = m_start + std::chrono::microseconds(timestamp - m_firstPresentationTime);
	m_end = (std::max)(m_end, target + std::chrono::microseconds(durationInMicroseconds));
	return std::max<int64_t>(0, std::chrono::duration_cast<std::chrono::microseconds>(target - now).count());
}

int64_t SessionSink::PlaybackClock::remaining() const
{
	if (!m_started) {
		return 0;
	}
	return std::max<int64_t>(0, std::chrono::duration_cast<std::chrono::microseconds>(
		m_end - std::chrono::steady_clock::now()).count());
}

SessionSink::SessionSink(UsageEnvironment& env, SessionCallback* callback, size_t bufferSize, PlaybackClock* clock)
	: MediaSink(env)
	, m_buffer(NULL)
	, m_bufferSize(bufferSize)
	, m_callback(callback) 
	, m_markerSize(0)
	, m_clock(clock)
{
}

SessionSink::~SessionSink()
{
	if (m_callback) {
		m_callback->onCloseSession(this->name());
	}
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
	m_readPending = false;
	if (numTruncatedBytes != 0)
	{
		delete [] m_buffer;
		envir() << "buffer too small " << (int)m_bufferSize << " allocate bigger one\n";
		allocate(m_bufferSize*2);
		this->continuePlaying();
		return;
	}
	if (!m_clock) {
		if (m_callback && !m_callback->onData(this->name(), m_buffer, frameSize+m_markerSize, presentationTime)) {
			envir() << "NOTIFY failed\n";
		}
		this->continuePlaying();
		return;
	}
	PendingFrame frame;
	frame.data.assign(m_buffer, m_buffer + frameSize + m_markerSize);
	frame.presentationTime = presentationTime;
	frame.duration = durationInMicroseconds;
	m_queuedBytes += frame.data.size();
	m_frames.push_back(std::move(frame));
	m_clock->delay(presentationTime, durationInMicroseconds);
	scheduleFrame();
	readIfReady();
}

void SessionSink::deliverFrame()
{
	nextTask() = nullptr;
	PendingFrame frame = std::move(m_frames.front());
	m_frames.pop_front();
	m_queuedBytes -= frame.data.size();
	if (m_callback && !m_callback->onData(this->name(), frame.data.data(), frame.data.size(), frame.presentationTime)) {
		envir() << "NOTIFY failed\n";
	}
	if (m_inputClosed && m_frames.empty()) {
		m_inputClosed = false;
		onSourceClosure();
	} else {
		scheduleFrame();
		readIfReady();
	}
}

void SessionSink::scheduleFrame()
{
	if (m_frames.empty() || nextTask() != nullptr) {
		return;
	}
	const PendingFrame& frame = m_frames.front();
	const int64_t delay = m_clock->delay(frame.presentationTime, frame.duration);
	nextTask() = envir().taskScheduler().scheduleDelayedTask(delay, deliverFrame, this);
}

void SessionSink::readIfReady()
{
	const size_t maxQueuedBytes = 16 * 1024 * 1024;
	const size_t maxQueuedFrames = 256;
	const int64_t maxReadAhead = 1000000;
	if (!m_readPending && !m_inputClosed && source()
	    && m_queuedBytes < maxQueuedBytes && m_frames.size() < maxQueuedFrames
	    && (m_frames.empty() || m_clock->delay(m_frames.back().presentationTime, m_frames.back().duration) < maxReadAhead)) {
		continuePlaying();
	}
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
		m_readPending = true;
		source()->getNextFrame(m_buffer+m_markerSize, m_bufferSize-m_markerSize,
				afterGettingFrame, this,
				onInputClosure, this);
		ret = True;
	}
	return ret;	
}
