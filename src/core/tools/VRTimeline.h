#ifndef VRTIMELINE_H_INCLUDED
#define VRTIMELINE_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "VRToolsFwd.h"
#include "core/utils/VRFunctionFwd.h"

#include <vector>

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRTimeline : public std::enable_shared_from_this<VRTimeline> {
	private:
	    struct SubTime {
            double t1 = 0;
            double t2 = 1;
            double T = 1;

            SubTime(double t1, double t2);

            bool before(double t);
            bool after(double t);
            bool active(double t);
            double interp(double t);
            double convert(double t);
	    };

	    template< typename T >
	    struct Entry {
	        T obj;
	        SubTime subTime;

	        Entry(double t1, double t2, T o) : obj(o), subTime(t1, t2) {}
            void update(double t);
	    };

	    double time = 0;
	    vector< Entry<VRAnimCbPtr> > callbacks;
	    vector< Entry<VRTimelinePtr> > timelines;

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
