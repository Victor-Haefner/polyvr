#ifndef VRDEVICEFWD_H_INCLUDED
#define VRDEVICEFWD_H_INCLUDED

#include <memory>

namespace OSG {
    class VRDevice;
    class VRSignal;
}

typedef std::shared_ptr<OSG::VRSignal> VRDevicePtr;
typedef std::weak_ptr<OSG::VRSignal> VRDeviceWeakPtr;
typedef std::shared_ptr<OSG::VRSignal> VRSignalPtr;
typedef std::weak_ptr<OSG::VRSignal> VRSignalWeakPtr;

#endif // VRDEVICEFWD_H_INCLUDED
