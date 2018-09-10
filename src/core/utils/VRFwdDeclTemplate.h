#ifndef VRFWDDECLTEMPLATE_H_INCLUDED
#define VRFWDDECLTEMPLATE_H_INCLUDED

#include <memory>

// fwd template for classes

#define ptrFwd( X ) \
class X; \
typedef std::shared_ptr<X> X ## Ptr; \
typedef std::weak_ptr<X> X ## WeakPtr; \
typedef std::unique_ptr<X> X ## UniquePtr;

#define ptrTemplateFwd( X, Y, T ) \
template<class K> class X; \
typedef X<T> Y; \
typedef std::shared_ptr<X<T>> Y ## Ptr; \
typedef std::weak_ptr<X<T>> Y ## WeakPtr; \
typedef std::unique_ptr<X<T>> Y ## UniquePtr;

// fwd template for functors
template<class T, class R = void> class VRFunction;

#define ptrFctFwd( X, T ) \
typedef VRFunction<T> X ## Cb; \
typedef std::shared_ptr<X ## Cb> X ## CbPtr; \
typedef std::weak_ptr<X ## Cb> X ## CbWeakPtr; \
typedef std::unique_ptr<X ## Cb> X ## CbUniquePtr;

#define ptrRFctFwd( X, T, R ) \
typedef VRFunction<T, R> X ## Cb; \
typedef std::shared_ptr<X ## Cb> X ## CbPtr; \
typedef std::weak_ptr<X ## Cb> X ## CbWeakPtr; \
typedef std::unique_ptr<X ## Cb> X ## CbUniquePtr;

#endif // VRFWDDECLTEMPLATE_H_INCLUDED
