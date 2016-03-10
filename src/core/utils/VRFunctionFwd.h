#ifndef VRFUNCTIONFWD_H_INCLUDED
#define VRFUNCTIONFWD_H_INCLUDED

#include <memory>

#define ptrFktFwd( X, T ) \
typedef VRFunction<T> X ## Cb; \
typedef std::shared_ptr<X ## Cb> X ## CbPtr; \
typedef std::weak_ptr<X ## Cb> X ## CbWeakPtr;

namespace OSG {
    class VRDevice;
}

class VRFunction_base;
template<class T> class VRFunction;

typedef std::shared_ptr<VRFunction_base> VRBaseCb;
typedef std::weak_ptr<VRFunction_base> VRBaseWeakCb;
typedef std::shared_ptr<VRFunction<int> > VRUpdatePtr;
typedef std::weak_ptr<VRFunction<int> > VRUpdateWeakPtr;
typedef std::shared_ptr<VRFunction<bool> > VRTogglePtr;
typedef std::weak_ptr<VRFunction<bool> > VRToggleWeakPtr;
typedef std::shared_ptr<VRFunction<float> > VRAnimPtr;
typedef std::weak_ptr<VRFunction<float> > VRAnimWeakPtr;
typedef std::shared_ptr<VRFunction<OSG::VRDevice*> > VRDeviceCb;
typedef std::weak_ptr<VRFunction<OSG::VRDevice*> > VRDeviceWeakCb;

#endif // VRFUNCTIONFWD_H_INCLUDED
