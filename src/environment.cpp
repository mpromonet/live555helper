/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** environment.cpp
** 
** -------------------------------------------------------------------------*/


#include "logger.h"

#include "environment.h"

Environment::Environment() : BasicUsageEnvironment(*BasicTaskScheduler::createNew()), m_stop(0)
{
}

Environment::~Environment()
{
	TaskScheduler* scheduler = &this->taskScheduler();
	this->reclaim();
	delete scheduler;	
}

void Environment::mainloop()
{
	this->taskScheduler().doEventLoop(&m_stop);	
}

void Environment::stop()
{
	m_stop = 1;	
}
