#ifndef VRCONCEPT_H_INCLUDED
#define VRCONCEPT_H_INCLUDED

#include "VROntologyUtils.h"
#include "VROntologyRule.h"

#include "addons/Semantics/VRSemanticsFwd.h"
#include "core/utils/VRName.h"

#include <map>
#include <vector>
#include <OpenSG/OSGConfig.h>

using namespace std;
OSG_BEGIN_NAMESPACE;

struct VROntologyLink : VRStorage {
    bool connected;
    string parent;
    string child;
    string parent_ontology;
    string child_ontology;

    VROntologyLink(VRConceptPtr parent = 0, VRConceptPtr child = 0);
    static VROntologyLinkPtr create(VRConceptPtr parent = 0, VRConceptPtr child = 0);
};

struct VRConcept : public std::enable_shared_from_this<VRConcept>, public VROntoID, public VRName {
    static map<int, VRConceptPtr> ConceptsByID;
    static map<string, VRConceptPtr> ConceptsByName;

    //VROntologyWeakPtr ontology;
    map<int, VRConceptPtr> parents;
    //map<int, VRConceptPtr> children;
    map<int, VRPropertyPtr> properties;
    map<int, VRPropertyPtr> annotations;
    map<int, VROntologyLinkPtr> links;
    vector<VROntologyRule*> rules;

    VRConcept(string name, VROntologyPtr o);
    ~VRConcept();
    static VRConceptPtr create(string name = "none", VROntologyPtr o = 0);
    VRConceptPtr ptr();

    VRConceptPtr copy();
    VRConceptPtr append(string name, bool link = false);
    void append(VRConceptPtr c, bool link = false);
    void removeChild(VRConceptPtr c);
    void removeParent(VRConceptPtr c);

    VRPropertyPtr addProperty(string name, string type);
    VRPropertyPtr addProperty(string name, VRConceptPtr c);
    void addProperty(VRPropertyPtr p);
    void remProperty(VRPropertyPtr p);
    void addAnnotation(VRPropertyPtr p);
    VRPropertyPtr addAnnotation(string name, string type);

    int getPropertyID(string name);
    VRPropertyPtr getProperty(string name, bool warn = true);
    vector<VRPropertyPtr> getProperties(string type);
    VRPropertyPtr getProperty(int ID);
    vector<VRPropertyPtr> getProperties();
    void getProperties(map<string, VRPropertyPtr>& res);

    bool hasParent(VRConceptPtr c = 0);
    vector<VRConceptPtr> getParents();
    //void getDescendance(map<int, VRConceptPtr>& concepts);
    void detach();

    bool is_a(VRConceptPtr c);
    bool is_a(string concept);
    string toString(string indent = "");
    string toString(map<int, vector<VRConceptPtr>>& cMap, string indent = "");

    void setup();
};

OSG_END_NAMESPACE;

#endif
