

#include "MediaSession.hh"

class MediaSessionHelper : public MediaSession
{
public:
	//MediaSessionHelper(UsageEnvironment &env) : MediaSession(UsageEnvironment env);

	static MediaSessionHelper *createNew(UsageEnvironment &env,
										 char const *sdpDescription);

	void setRangeClockAttribute(const char *absStartTime = nullptr, const char *absEndTime = nullptr);

protected:
	MediaSessionHelper(UsageEnvironment &env);

};