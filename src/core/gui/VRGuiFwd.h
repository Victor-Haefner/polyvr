#ifndef VRGUIFWD_H_INCLUDED
#define VRGUIFWD_H_INCLUDED

#include <memory>

#define ptrFwd( X ) \
class X; \
typedef std::shared_ptr<X> X ## Ptr; \
typedef std::weak_ptr<X> X ## WeakPtr;

#define ptrFwd2( X, Y ) \
class Y; \
typedef std::shared_ptr<Y> X ## Ptr; \
typedef std::weak_ptr<Y> X ## WeakPtr;

namespace OSG {

ptrFwd(VRGuiTreeExplorer);

}

namespace Gtk {

ptrFwd(Window);

}

#endif // VRGUIFWD_H_INCLUDED
