#ifndef VRENTITY_H_INCLUDED
#define VRENTITY_H_INCLUDED

#include "VROntologyUtils.h"
#include "VRConcept.h"
#include "core/utils/VRName.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

struct VREntity : public VROntoID, public VRName {
    vector<VRConceptWeakPtr> concepts;
    vector<string> conceptNames;
    map<string, vector<VRPropertyPtr> > properties;
    VROntologyWeakPtr ontology;

    VREntity(string name, VROntologyPtr o, VRConceptPtr c = 0);

    static VREntityPtr create(string name = "none", VROntologyPtr o = 0, VRConceptPtr c = 0);
    void addConcept(VRConceptPtr c);
    vector<VRConceptPtr> getConcepts();
    vector<string> getConceptNames();
    VRPropertyPtr getProperty(string p);
    vector<VRPropertyPtr> getProperties();
    string getConceptList();

    void set(string name, string value);
    void add(string name, string value);
    void setVector(string name, vector<string> value, string type);
    void addVector(string name, vector<string> value, string type);
    string get(string prop, int i = 0);
    string getVector(string prop, int i = 0);
    void rem(VRPropertyPtr);
    string toString();

    vector<VRPropertyPtr> getValues(string name = "");
    VRPropertyPtr getValue(string name);

    vector<string> getAtPath(vector<string> path);

    void save(xmlpp::Element* e, int p);
    void load(xmlpp::Element* e);
};

OSG_END_NAMESPACE;

#endif
