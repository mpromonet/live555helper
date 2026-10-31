/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** environment.cpp
** 
** -------------------------------------------------------------------------*/

#include <iostream>
#include "environment.h"


Environment::Environment() : BasicUsageEnvironment(*BasicTaskScheduler::createNew()), m_stop(0)
{
}

Environment::~Environment()
{
	TaskScheduler* scheduler = &this->taskScheduler();
	delete scheduler;	
}

void Environment::mainloop()
{
	this->taskScheduler().doEventLoop(&m_stop);	
}

void Environment::stop()
{
	this->taskScheduler().scheduleDelayedTask(0, [](void* env) { static_cast<Environment*>(env)->m_stop = 1; }, this);
}
