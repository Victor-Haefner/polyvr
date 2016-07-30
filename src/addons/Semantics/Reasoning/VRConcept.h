#ifndef VRCONCEPT_H_INCLUDED
#define VRCONCEPT_H_INCLUDED

#include "VROntologyUtils.h"
#include "VROntologyRule.h"

#include "addons/Semantics/VRSemanticsFwd.h"

#include <map>
#include <vector>
#include <OpenSG/OSGConfig.h>

using namespace std;
OSG_BEGIN_NAMESPACE;

struct VRConcept : public std::enable_shared_from_this<VRConcept>, public VRNamedID {
    VRConceptWeakPtr parent;
    map<int, VRConceptPtr> children;
    map<int, VRPropertyPtr> properties;
    map<int, VRPropertyPtr> annotations;
    vector<VROntologyRule*> rules;

    VRConcept(string name);
    static VRConceptPtr create(string name);

    VRConceptPtr append(string name);
    void append(VRConceptPtr c);

    VRPropertyPtr addProperty(string name, string type);
    void addProperty(VRPropertyPtr p);
    void addAnnotation(VRPropertyPtr p);

    int getPropertyID(string name);
    VRPropertyPtr getProperty(string name);
    vector<VRPropertyPtr> getProperties(string type);
    VRPropertyPtr getProperty(int ID);
    vector<VRPropertyPtr> getProperties();
    void getProperties(map<string, VRPropertyPtr>& res);

    bool is_a(string concept);
    string toString(string indent = "");
};

OSG_END_NAMESPACE;

#endif
