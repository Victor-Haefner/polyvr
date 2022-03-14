#ifndef VRLADVARIABLE_H_INCLUDED
#define VRLADVARIABLE_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "addons/Engineering/VREngineeringFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRLADVariable : public std::enable_shared_from_this<VRLADVariable> {
	public:
        string name;
        string logicalAddress;
        string dataType;
        string source;
        string remanence;
        int value = 0;
        int startValue = 0;

	public:
		VRLADVariable();
		~VRLADVariable();

		static VRLADVariablePtr create();
		VRLADVariablePtr ptr();
};

OSG_END_NAMESPACE;

#endif //VRLADVARIABLE_H_INCLUDED
