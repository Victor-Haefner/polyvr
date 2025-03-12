#ifndef VRTIMELINE_H_INCLUDED
#define VRTIMELINE_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "VRToolsFwd.h"
#include "core/utils/VRFunctionFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRTimeline : public std::enable_shared_from_this<VRTimeline> {
	private:
	    double time = 0;

	public:
		VRTimeline();
		~VRTimeline();

		static VRTimelinePtr create();
		VRTimelinePtr ptr();

		void addCallback(double t1, double t2, VRAnimCbPtr cb);
		void addTimeline(double t1, double t2, VRTimelinePtr tl);

		void setTime(double t);
};

OSG_END_NAMESPACE;

#endif //VRTIMELINE_H_INCLUDED
