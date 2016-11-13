#ifndef VRFUNCTIONFWD_H_INCLUDED
#define VRFUNCTIONFWD_H_INCLUDED

#include "VRFwdDeclTemplate.h"

class VRFunction_base;
template<class T> class VRFunction;
typedef std::shared_ptr<VRFunction_base> VRBaseCbPtr;
typedef std::weak_ptr<VRFunction_base> VRBaseCbWeakPtr;

namespace OSG {
    ptrFwd( VRDevice );
    ptrFwd( VRThread );
}

ptrFctFwd( VRUpdate, int );
ptrFctFwd( VRToggle, bool );
ptrFctFwd( VREval, bool& );
ptrFctFwd( VRAnim, float );
ptrFctFwd( VRDevice, OSG::VRDeviceWeakPtr );
ptrFctFwd( VRThread, OSG::VRThreadWeakPtr );
ptrFctFwd( VRServer, std::string );

#endif // VRFUNCTIONFWD_H_INCLUDED
