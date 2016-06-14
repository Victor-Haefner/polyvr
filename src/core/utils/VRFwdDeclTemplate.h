#ifndef VRFWDDECLTEMPLATE_H_INCLUDED
#define VRFWDDECLTEMPLATE_H_INCLUDED

#include <memory>

#define ptrFwd( X ) \
class X; \
typedef std::shared_ptr<X> X ## Ptr; \
typedef std::weak_ptr<X> X ## WeakPtr;

#endif // VRFWDDECLTEMPLATE_H_INCLUDED
