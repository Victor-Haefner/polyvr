#ifndef VRENTITY_H_INCLUDED
#define VRENTITY_H_INCLUDED

#include "core/math/OSGMathFwd.h"
#include "VROntologyUtils.h"
#include "VRProperty.h"
#include "core/utils/VRName.h"
#include "core/utils/toString.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

struct VRPropertyValue {// allows to cast type in py bindings
    VRPropertyPtr p;
    VROntologyPtr o;
    VRPropertyValue(VRPropertyPtr p, VROntologyPtr o) : p(p), o(o) {}
    VRPropertyValue() {}
};

struct VREntity : public VROntoID, public VRName {
    vector<VRConceptWeakPtr> concepts;
    vector<string> conceptNames;
    map<string, map<int, VRPropertyPtr> > properties;
    VROntologyWeakPtr ontology;
    VRObjectWeakPtr sgObject;

    VREntity(string name, VROntologyPtr o, VRConceptPtr c = 0);
    VREntity();

    static VREntityPtr create(string name = "none", VROntologyPtr o = 0, VRConceptPtr c = 0);
    VREntityPtr copy();

    VROntologyPtr getOntology();

    void addConcept(VRConceptPtr c);
    VRConceptPtr getConcept();
    vector<VRConceptPtr> getConcepts();
    vector<string> getConceptNames();
    bool hasProperty(string p);
    VRPropertyPtr getProperty(string p, bool warn = true);
    vector<VRPropertyPtr> getProperties();
    void rem(VRPropertyPtr);
    string getConceptList();

    void setSGObject(VRObjectPtr o);
    VRObjectPtr getSGObject();

    void addProperty(VRPropertyPtr prop, string name, string value, int pos = 0);
    void set(string prop, string value, int pos = 0);
    void add(string prop, string value);
    void clear(string prop);
    void setVector(string prop, vector<string> value, string type, int pos = 0);
    void addVector(string prop, vector<string> value, string type);
    void setVec3(string prop, Vec3d value, string type, int pos = 0);
    void addVec3(string prop, Vec3d value, string type);
    string asVectorString();

    VRPropertyPtr get(const string& prop, int i = 0);
    vector<VRPropertyPtr> getAll(string prop = "");
    vector<VRPropertyPtr> getVector(const string& prop, int i = 0);
    vector< vector<VRPropertyPtr> > getAllVector(const string& prop);

    VRPropertyValue getStringValue(string prop, int i = 0);
    vector<VRPropertyValue> getAllStringValues(string prop = "");
    vector<VRPropertyValue> getStringVector(string prop, int i = 0);
    vector< vector<VRPropertyValue> > getAllStringVector(string prop);

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

    bool is_a(string concept_);
    string toString();
    void save(XMLElementPtr e, int p) override;
    void load(XMLElementPtr e, VRStorageContextPtr context = 0) override;
};

OSG_END_NAMESPACE;

#endif
