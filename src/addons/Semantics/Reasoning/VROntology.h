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
    map<int, VROntologyRulePtr> rules;
    map<string, VROntologyWeakPtr> dependencies;
    map<string, VRCallbackStrWrapperPtr> builtins;

    map<string, VRConceptPtr> recentConcepts; // performance optimization

    VROntology(string name);
    static VROntologyPtr create(string name = "");
    VROntologyPtr ptr();

    void setup(VRStorageContextPtr context);

    void import(VROntologyPtr o);
    void merge(VROntologyPtr o);
    VROntologyPtr copy();
    map<int, vector<VRConceptPtr>> getChildrenMap();

    void addConcept(VRConceptPtr c);
    void addEntity(VREntityPtr& e);
    void remConcept(VRConceptPtr c);
    void remEntity(VREntityPtr e);
    void remEntities(string concept);
    void remRule(VROntologyRulePtr rule);
    void renameConcept(VRConceptPtr c, string newName);
    void renameEntity(VREntityPtr e, string s);

    VRConceptPtr addConcept(string concept, string parent = "", string comment = "");
    VROntologyRulePtr addRule(string rule, string ac);
    VREntityPtr addEntity(string name, string concept);
    VREntityPtr addVectorEntity(string name, string concept, string x, string y, string z);
    VREntityPtr addVectorEntity(string name, string concept, vector<string> val);

    template <typename T, typename R, typename ...Args>
    VRCallbackStrWrapperPtr addBuiltin(string builtin, R (T::*callback)(Args...) );

    vector<VRConceptPtr> getConcepts();
    VRConceptPtr getConcept(string name);
    VREntityPtr getEntity(int ID);
    VREntityPtr getEntity(string instance);
    vector<VREntityPtr> getEntities(string concept);

    vector<VROntologyRulePtr> getRules();

    void openOWL(string path);
    void saveOWL(string path);
    void addModule(string mod);
    string toString();

    void setFlag(string f);
    string getFlag();

    vector<VREntityPtr> process(string query);
};

OSG_END_NAMESPACE;

#endif // VRONTOLOGY_H_INCLUDED
