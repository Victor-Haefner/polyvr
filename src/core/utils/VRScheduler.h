#ifndef VRSCHEDULER_H_INCLUDED
#define VRSCHEDULER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <functional>
#include "VRUtilsFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRScheduler : public std::enable_shared_from_this<VRScheduler> {
	private:
        vector< function<void()> > postponed;

	public:
		VRScheduler();
		~VRScheduler();

		static VRSchedulerPtr create();
		VRSchedulerPtr ptr();

		void postpone(function<void()> f);
		size_t getNPostponed();
		void callPostponed(bool recall = false, int limit = 10);
};

OSG_END_NAMESPACE;

#endif //VRSCHEDULER_H_INCLUDED
