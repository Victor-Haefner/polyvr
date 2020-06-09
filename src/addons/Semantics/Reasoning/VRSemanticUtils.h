#ifndef VRSEMANTICUTILS_H_INCLUDED
#define VRSEMANTICUTILS_H_INCLUDED

#include <string>
#include <vector>
#include <map>
#include <list>
#include <OpenSG/OSGConfig.h>

#include "../VRSemanticsFwd.h"
#include "core/utils/VRFunctionFwd.h"
#include "core/objects/VRObjectFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

struct VPath {
    string first;
    string root;
    vector<string> nodes;

    VPath(string p);
    string toString();
    int size();

    vector<string> getValue(VREntityPtr e);
    void setValue(string v, VREntityPtr e);
};

struct Evaluation {
    enum STATE {ALL, VALID, INVALID, ASSUMPTION};
    STATE state = VALID; // valid, assumption, anonymous, ruled_out, ...
    string toString();
};

struct Variable {
    map<int, VREntityPtr> entities;
    map<int, Evaluation> evaluations; // summarize the evaluation state of each instance!
    vector<string> value;
    string concept = "var";
    bool isAnonymous = true;
    bool valid = true;

    string toString();
    string valToString();
    bool has(VariablePtr other, VPath& p1, VPath& p2, VROntologyPtr onto);
    bool is(VariablePtr other, VPath& p1, VPath& p2);

    Variable(VROntologyPtr onto, string concept, vector<string> var, VRSemanticContextPtr context);
    Variable(VROntologyPtr onto, vector<string> val);
    Variable();

    static VariablePtr create(VROntologyPtr onto, string concept, vector<string> var, VRSemanticContextPtr context);
    static VariablePtr create(VROntologyPtr onto, vector<string> val);

    void addEntity(VREntityPtr e, bool assumtion = false);
    bool operator==(Variable v);
    void discard(VREntityPtr e);

    void addAssumption(VRSemanticContextPtr context, string var);

    vector<VREntityPtr> getEntities(Evaluation::STATE state);
};

struct Term {
    VPath path;
    VariablePtr var = 0;
    string str;

    Term(string s);
    bool valid();

    bool isMathExpression();
    vector<string> computeMathExpression(VRSemanticContextPtr c);

    bool is(Term& t, VRSemanticContextPtr context);
    bool has(Term& t, VRSemanticContextPtr context);
};

struct Query {
    VRStatementPtr request;
    vector<VRStatementPtr> statements;

    Query();
    Query(string q);
    string toString();

    void checkState();
    void substituteRequest(VRStatementPtr s);
};

struct Constructor {
    Query query;
};

struct VRSemanticContext : public std::enable_shared_from_this<VRSemanticContext>{
    map<string, bool> options;
    map<string, VariablePtr> vars;
    map<string, Query> rules;
    list<Query> queries;
    vector<VREntityPtr> results;
    VROntologyPtr onto = 0;

    int itr=0;
    int itr_stale=0;
    int itr_max = 20;
    int itr_max_stale = 2;

    VRSemanticContext(VROntologyPtr onto = 0);
    VRSemanticContextPtr ptr();
    static VRSemanticContextPtr create(VROntologyPtr onto = 0);

    void init();
    bool getOption(string option);
};

OSG_END_NAMESPACE;

#endif // VRSEMANTICUTILS_H_INCLUDED
