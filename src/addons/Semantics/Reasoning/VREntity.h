#ifndef VRENTITY_H_INCLUDED
#define VRENTITY_H_INCLUDED

#include "VROntologyUtils.h"
#include "VRConcept.h"
#include "core/utils/VRName.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

struct VREntity : public VROntoID, public VRName {
    VRConceptWeakPtr concept;
    string conceptName;
    map<string, vector<VRPropertyPtr> > properties;

    VREntity(string name, VRConceptPtr c = 0);
    static VREntityPtr create(string name = "none", VRConceptPtr c = 0);
    void setConcept(VRConceptPtr c);
    VRConceptPtr getConcept();

    void set(string name, string value);
    void setVector(string name, vector<string> value, string type);
    void add(string name, string value);
    string toString();

    vector<VRPropertyPtr> getProperties(string name = "");

    vector<string> getAtPath(vector<string> path);
};

OSG_END_NAMESPACE;

#endif
