#ifndef VRFUNCTIONFWD_H_INCLUDED
#define VRFUNCTIONFWD_H_INCLUDED

#include <memory>

#define ptrFktFwd( X, T ) \
typedef VRFunction<T> X ## Cb; \
typedef std::shared_ptr<X ## Cb> X ## CbPtr; \
typedef std::weak_ptr<X ## Cb> X ## CbWeakPtr;

class VRFunction_base;
template<class T> class VRFunction;

namespace OSG {
    class VRDevice;
    typedef std::shared_ptr<VRDevice> VRDevicePtr;
    typedef std::weak_ptr<VRDevice> VRDeviceWeakPtr;

    class VRThread;
    typedef std::shared_ptr<VRThread> VRThreadPtr;
    typedef std::weak_ptr<VRThread> VRThreadWeakPtr;
    typedef std::shared_ptr< VRFunction< VRThreadWeakPtr > > VRThreadCb;
    typedef std::weak_ptr< VRFunction< VRThreadWeakPtr > > VRThreadWeakCb;
}

typedef std::shared_ptr<VRFunction_base> VRBaseCb;
typedef std::weak_ptr<VRFunction_base> VRBaseWeakCb;
typedef std::shared_ptr<VRFunction<int> > VRUpdatePtr;
typedef std::weak_ptr<VRFunction<int> > VRUpdateWeakPtr;
typedef std::shared_ptr<VRFunction<bool> > VRTogglePtr;
typedef std::weak_ptr<VRFunction<bool> > VRToggleWeakPtr;
typedef std::shared_ptr<VRFunction<float> > VRAnimPtr;
typedef std::weak_ptr<VRFunction<float> > VRAnimWeakPtr;
typedef std::shared_ptr<VRFunction<OSG::VRDeviceWeakPtr> > VRDeviceCb;
typedef std::weak_ptr<VRFunction<OSG::VRDeviceWeakPtr> > VRDeviceWeakCb;

#endif // VRFUNCTIONFWD_H_INCLUDED
