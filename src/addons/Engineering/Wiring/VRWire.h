#ifndef VRWIRE_H_INCLUDED
#define VRWIRE_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "addons/Semantics/VRSemanticsFwd.h"
#include "addons/Engineering/VREngineeringFwd.h"
#include "VRElectricComponent.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRWire : public std::enable_shared_from_this<VRWire> {
	public:
        VREntityPtr entity;
        VRElectricComponent::Address source;
        VRElectricComponent::Address target;
        string label = "Default";
        string cType = "none";

	public:
		VRWire();
		~VRWire();

		static VRWirePtr create();
		VRWirePtr ptr();

		VRElectricComponent::Address getThis(VRElectricComponentPtr first);
        VRElectricComponent::Address getOther(VRElectricComponentPtr first);
};

OSG_END_NAMESPACE;

#endif //VRWIRE_H_INCLUDED
