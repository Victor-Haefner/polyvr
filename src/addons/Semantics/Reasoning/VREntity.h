#ifndef VRENTITY_H_INCLUDED
#define VRENTITY_H_INCLUDED

#include <OpenSG/OSGVector.h>
#include "VROntologyUtils.h"
#include "VRProperty.h"
#include "core/utils/VRName.h"
#include "core/utils/toString.h"

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
    VREntityPtr copy();
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
    void setVec3(string prop, Vec3d value, string type, int pos = 0);
    void addVec3(string prop, Vec3d value, string type);

    VRPropertyPtr get(const string& prop, int i = 0);
    vector<VRPropertyPtr> getAll(const string& prop = "");
    vector<VRPropertyPtr> getVector(const string& prop, int i = 0);
    vector< vector<VRPropertyPtr> > getAllVector(const string& prop);

    template<class T> T getValue(const string& prop, T t, int i = 0) {
        auto P = get(prop, i);
        if (P) toValue(P->value, t);
        return t;
    }

    template<class T> vector<T> getAllValues(const string& prop) {
        vector<T> v;
        for (auto p : getAll(prop)) {
            T t;
            toValue(p->value, t);
            v.push_back(t);
        }
        return v;
    }

    VREntityPtr getEntity(const string& prop, int i = 0);
    vector<VREntityPtr> getAllEntities(const string& prop = "");
    Vec3d getVec3(const string& prop, int i = 0);
    vector< Vec3d > getAllVec3(const string& prop);

    bool is_a(const string& concept);
    string toString();
    void save(xmlpp::Element* e, int p);
    void load(xmlpp::Element* e);
};

OSG_END_NAMESPACE;

#endif
