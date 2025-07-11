#ifndef VRONTOLOGY_H_INCLUDED
#define VRONTOLOGY_H_INCLUDED

#include "VRConcept.h"
#include "VROntologyRule.h"
#include "VREntity.h"
#include "../VRSemanticsFwd.h"
#include "core/utils/VRName.h"
#include "core/utils/VRUtilsFwd.h"

#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <memory>

using namespace std;
OSG_BEGIN_NAMESPACE;

struct VROntology : public std::enable_shared_from_this<VROntology>, public VRName {
    static map<string, VROntologyPtr> library;
    static void setupLibrary();

    string flag = "built-in";

    VRConceptPtr thing = 0;
    map<int, VREntityPtr> entities;
    map<string, VREntityPtr> entitiesByName;
    map<string, VRConceptWeakPtr> concepts;
    map<string, VRPropertyPtr> properties;
    map<int, VROntologyRulePtr> rules;
    map<string, VROntologyWeakPtr> dependencies;
    map<string, VRCallbackStrWrapperPtr> builtins;
    map<string, VROntologyWeakPtr> modules;
    map<string, VRConceptPtr> recentConcepts; // performance optimization

    VROntology(string name);
    static VROntologyPtr create(string name = "");
    VROntologyPtr ptr();

    void setup(VRStorageContextPtr context);

    void merge(VROntologyPtr o);
    VROntologyPtr copy();
    void addModule(VROntologyPtr onto);
    void addModule(string mod);
    void loadModule(string path);
    map<string, VROntologyPtr> getModules();
    map<int, vector<VRConceptPtr>> getChildrenMap();

    void addConcept(VRConceptPtr c);
    void addEntity(VREntityPtr& e);
    void addProperty(VRPropertyPtr p);
    void remConcept(VRConceptPtr c);
    void remEntity(VREntityPtr e);
    void remEntity(string name);
    void remEntities(string concept_);
    void remRule(VROntologyRulePtr rule);
    void renameConcept(VRConceptPtr c, string newName);
    void renameEntity(VREntityPtr e, string s);

    VRConceptPtr addConcept(string concept_, string parent = "", map<string, string> props = map<string, string>(), string comment = "");
    VROntologyRulePtr addRule(string rule, string ac);
    VREntityPtr addEntity(string name, string concept_ = "", map<string, string> props = map<string, string>());
    VREntityPtr addVectorEntity(string name, string concept_, string x, string y, string z);
    VREntityPtr addVectorEntity(string name, string concept_, vector<string> val);
    VREntityPtr addVec3Entity(string name, string concept_, Vec3d v);

    template <typename T, typename R, typename ...Args>
    VRCallbackStrWrapperPtr addBuiltin(string builtin, R (T::*callback)(Args...) );

    vector<VRConceptPtr> getConcepts();
    VRConceptPtr getConcept(string name);
    VREntityPtr getEntity(int ID);
    VREntityPtr getEntity(string instance);
    VRPropertyPtr getProperty(string prop);
    vector<VREntityPtr> getEntities(string concept_);
    vector<VROntologyRulePtr> getRules();

    void openOWL(string path);
    void saveOWL(string path);
    string toString();

    void setFlag(string f);
    string getFlag();

    vector<VREntityPtr> process(string query, bool allowAssumptions = false);
};

OSG_END_NAMESPACE;

#endif // VRONTOLOGY_H_INCLUDED
