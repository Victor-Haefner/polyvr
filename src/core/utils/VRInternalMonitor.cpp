#include "VRInternalMonitor.h"
#include "core/scene/VRSceneManager.h"
#include "toString.h"
#include "VRFunction.h"
#include "core/objects/VRTransform.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

void nameDictSize(string& s) { s = toString(VRName::getBaseNameNumber()); }
void nameBaseDictSize(string& s) { s = toString(VRName::getNameNumber()); }
void dynObjctsSize(string& s) { s = toString((int)VRTransform::dynamicObjects.size()); }
void chgObjctsSize(string& s) { s = toString((int)VRTransform::changedObjects.size()); }

VRInternalMonitor::VRInternalMonitor() {
    //VRFunction<int>* fkt = new VRFunction<int>( "Internal_Monitor_update", boost::bind(&VRInternalMonitor::update, this) );
    //VRSceneManager::get()->addUpdateFkt(fkt);

    add("VRName::nameDict size", new varFkt("Set_test_var", nameDictSize));
    add("VRName::nameDict full size", new varFkt("Set_test_var", nameBaseDictSize));
    add("VRTransform::dynamicObjects size", new varFkt("Set_test_var", dynObjctsSize));
    add("VRTransform::changedObjects size", new varFkt("Set_test_var", chgObjctsSize));
}

VRInternalMonitor* VRInternalMonitor::get() {
    static VRInternalMonitor* sng = new VRInternalMonitor();
    return sng;
}

void VRInternalMonitor::add( string name, VRFunction<string&>* fkt ) {
    varFkts[name] = fkt;
}

void VRInternalMonitor::update() {
    string val;
    for(auto var : varFkts) (*var.second)(val);
}

map<string, string> VRInternalMonitor::getVariables() {
    string val;
    map<string, string> m;
    for(auto var : varFkts) {
        (*var.second)(val);
        m[var.first] = val;
    }
    return m;
}

OSG_END_NAMESPACE;
