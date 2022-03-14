#include "VRLADVariable.h"

using namespace OSG;

VRLADVariable::VRLADVariable() {}
VRLADVariable::~VRLADVariable() {}

VRLADVariablePtr VRLADVariable::create() { return VRLADVariablePtr( new VRLADVariable() ); }
VRLADVariablePtr VRLADVariable::ptr() { return static_pointer_cast<VRLADVariable>(shared_from_this()); }

void VRLADVariable::setName(string n) { name = n; }
void VRLADVariable::setLogicalAddress(string a) { logicalAddress = a; }
void VRLADVariable::setDataType(string t) { dataType = t; }
void VRLADVariable::setSource(string s) { source = s; }
void VRLADVariable::setRemanence(string r) { remanence = r; }
void VRLADVariable::setValue(string v) { value = v; }
void VRLADVariable::setStartValue(string v) { startValue = v; }

string VRLADVariable::getName() { return name; }
string VRLADVariable::getLogicalAddress() { return logicalAddress; }
string VRLADVariable::getDataType() { return dataType; }
string VRLADVariable::getSource() { return source; }
string VRLADVariable::getRemanence() { return remanence; }
string VRLADVariable::getValue() { return value; }
string VRLADVariable::getStartValue() { return startValue; }
