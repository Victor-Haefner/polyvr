#ifndef VRDEVICEFWD_H_INCLUDED
#define VRDEVICEFWD_H_INCLUDED

#include <memory>

#define ptrFwd( X ) \
class X; \
typedef std::shared_ptr<X> X ## Ptr; \
typedef std::weak_ptr<X> X ## WeakPtr;

namespace OSG {

ptrFwd(VRSignal);
ptrFwd(VRDevice);
ptrFwd(VRFlystick);
ptrFwd(VRMouse);
ptrFwd(VRKeyboard);
ptrFwd(VRServer);
ptrFwd(VRHaptic);
ptrFwd(ART);
ptrFwd(ART_device);
ptrFwd(VRPN);
ptrFwd(VRPN_device);
ptrFwd(VRLeap);
ptrFwd(VRLeapFrame);

}

#endif // VRDEVICEFWD_H_INCLUDED
