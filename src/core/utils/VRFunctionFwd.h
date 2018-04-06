#ifndef VRFUNCTIONFWD_H_INCLUDED
#define VRFUNCTIONFWD_H_INCLUDED

#include "VRFwdDeclTemplate.h"

#include <string>
#include <map>

class VRFunction_base;
typedef std::shared_ptr<VRFunction_base> VRBaseCbPtr;
typedef std::weak_ptr<VRFunction_base> VRBaseCbWeakPtr;

namespace OSG {
    ptrFwd( VRDevice );
    ptrFwd( VRThread );
}

ptrFctFwd( VRUpdate, void );
ptrFctFwd( VRToggle, bool );
ptrFctFwd( VREval, bool& );
ptrFctFwd( VRAnim, float );
ptrFctFwd( VRMessage, std::string );
ptrFctFwd( VRDevice, OSG::VRDeviceWeakPtr );
ptrFctFwd( VRThread, OSG::VRThreadWeakPtr );

typedef std::map<std::string, std::string> strMap;
ptrRFctFwd( VRServer, strMap, std::string );

#endif // VRFUNCTIONFWD_H_INCLUDED
