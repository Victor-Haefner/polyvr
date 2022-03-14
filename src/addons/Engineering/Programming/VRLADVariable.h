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
        string value = "0";
        string startValue = "0";

	public:
		VRLADVariable();
		~VRLADVariable();

		static VRLADVariablePtr create();
		VRLADVariablePtr ptr();

		void setName(string name);
		void setLogicalAddress(string logicalAddress);
		void setDataType(string dataType);
		void setSource(string source);
		void setRemanence(string remanence);
		void setValue(string value);
		void setStartValue(string startValue);

		string getName();
		string getLogicalAddress();
		string getDataType();
		string getSource();
		string getRemanence();
		string getValue();
		string getStartValue();
};

OSG_END_NAMESPACE;

#endif //VRLADVARIABLE_H_INCLUDED
