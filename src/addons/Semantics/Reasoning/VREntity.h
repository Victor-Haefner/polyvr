#ifndef VRENTITY_H_INCLUDED
#define VRENTITY_H_INCLUDED

#include <OpenSG/OSGVector.h>
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
    VRObjectWeakPtr sgObject;

    VREntity(string name, VROntologyPtr o, VRConceptPtr c = 0);

    static VREntityPtr create(string name = "none", VROntologyPtr o = 0, VRConceptPtr c = 0);
    void addConcept(VRConceptPtr c);
    vector<VRConceptPtr> getConcepts();
    vector<string> getConceptNames();
    VRPropertyPtr getProperty(string p);
    vector<VRPropertyPtr> getProperties();
    void rem(VRPropertyPtr);
    string getConceptList();

    void setSGObject(VRObjectPtr o);
    VRObjectPtr getSGObject();

    void set(string prop, string value, int pos = 0);
    void add(string prop, string value);
    void clear(string prop);
    void setVector(string prop, vector<string> value, string type, int pos = 0);
    void addVector(string prop, vector<string> value, string type);

    VRPropertyPtr get(string prop, int i = 0);
    vector<VRPropertyPtr> getAll(string prop = "");
    vector<VRPropertyPtr> getVector(string prop, int i = 0);
    vector< vector<VRPropertyPtr> > getAllVector(string prop);

    VREntityPtr getEntity(string prop, int i = 0);
    vector<VREntityPtr> getAllEntities(string prop = "");
    Vec3f getVec3f(string prop, int i = 0);
    vector< Vec3f > getAllVec3f(string prop);

    bool is_a(string concept);
    string toString();
    void save(xmlpp::Element* e, int p);
    void load(xmlpp::Element* e);
};

OSG_END_NAMESPACE;

#endif
