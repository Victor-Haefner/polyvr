#ifndef VRFUNCTIONFWD_H_INCLUDED
#define VRFUNCTIONFWD_H_INCLUDED

#include <memory>

template<class T> class VRFunction;

typedef std::shared_ptr<VRFunction<int> > VRUpdatePtr;
typedef std::weak_ptr<VRFunction<int> > VRUpdateWeakPtr;
typedef std::shared_ptr<VRFunction<bool> > VRTogglePtr;
typedef std::weak_ptr<VRFunction<bool> > VRToggleWeakPtr;
typedef std::shared_ptr<VRFunction<float> > VRAnimPtr;
typedef std::weak_ptr<VRFunction<float> > VRAnimWeakPtr;

#endif // VRFUNCTIONFWD_H_INCLUDED
