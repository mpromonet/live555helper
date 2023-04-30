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

#if defined(_MSC_VER)
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

class Environment : public BasicUsageEnvironment
{
	public:
		Environment();
		Environment(char& stop);
		virtual ~Environment();
	
	
		void mainloop();
		void stop();	
		
	protected:
		char& m_stop;		
		char  m_stopRef;		
};

