/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 3 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// "liveMedia"
// Copyright (c) 1996-2019 Live Networks, Inc.  All rights reserved.
// A data structure that represents a session that consists of
// potentially multiple (audio and/or video) sub-sessions
// Implementation

#include "MediaSessionHelper.h"
#include "MediaSession.hh"

#include <iostream>
#include <sstream>

MediaSessionHelper::MediaSessionHelper(UsageEnvironment &env) : MediaSession(env)
{
	std::cout << " MediaSessionHelper constructor \n";
}


MediaSessionHelper *MediaSessionHelper::createNew(UsageEnvironment &env,
												  char const *sdpDescription)
{
	MediaSessionHelper *newSession = new MediaSessionHelper(env);
	if (newSession != NULL)
	{
		if (!newSession->initializeWithSDP(sdpDescription))
		{
			delete newSession;
			return NULL;
		}
	}

	return newSession;
}

// Force to set the start time and the end time at format "YYYMMDDTHHMMSSZ"
// use when SDP do not contains a=range: clock, but can use it, [ CASD Visimax ]
void MediaSessionHelper::setRangeClockAttribute(const char *absStartTime, const char *absEndTime)
{
	if (absStartTime)
	{
		if (fAbsStartTime)
		{
			free(fAbsStartTime);
		}
		fAbsStartTime = strdup(absStartTime);
	}
	if (absEndTime)
	{
		if (fAbsEndTime)
		{
			free(fAbsEndTime);
		}
		fAbsEndTime = strdup(absEndTime);
	}
}
