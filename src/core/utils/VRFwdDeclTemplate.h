#ifndef VRFWDDECLTEMPLATE_H_INCLUDED
#define VRFWDDECLTEMPLATE_H_INCLUDED

#include <memory>

// fwd template for classes

#define ptrFwd( X ) \
class X; \
typedef std::shared_ptr<X> X ## Ptr; \
typedef std::weak_ptr<X> X ## WeakPtr;

// fwd template for functors

#define ptrFctFwd( X, T ) \
typedef VRFunction<T> X ## Cb; \
typedef std::shared_ptr<X ## Cb> X ## CbPtr; \
typedef std::weak_ptr<X ## Cb> X ## CbWeakPtr;

#endif // VRFWDDECLTEMPLATE_H_INCLUDED
