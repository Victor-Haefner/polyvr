#include "VRMutex.h"

#include "core/utils/Thread.h"
#include <mutex>

using namespace OSG;
using namespace std;

namespace OSG {
    struct InnerMutex {
        recursive_mutex mtx;
        InnerMutex() {}
    };
}

VRMutex::VRMutex() { mutex = new InnerMutex(); }
VRMutex::~VRMutex() { delete mutex; }
InnerMutex* VRMutex::mtx() { return mutex; }

VRLock::VRLock(VRMutex& m) : mutex(m.mtx()) { mutex->mtx.lock(); }
VRLock::~VRLock() { mutex->mtx.unlock(); }
