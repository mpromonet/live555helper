/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** environment.h
** 
** -------------------------------------------------------------------------*/

#pragma once

#include "BasicUsageEnvironment.hh"


class Environment : public BasicUsageEnvironment
{
	public:
		Environment();
		virtual ~Environment();
	
	
		void mainloop()
		{
			this->taskScheduler().doEventLoop(&m_stop);
		}
				
		void stop() { m_stop = 1; };	
		
	protected:
		char                     m_stop;		
};

