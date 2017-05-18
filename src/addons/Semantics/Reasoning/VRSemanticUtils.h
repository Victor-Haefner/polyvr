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

struct VRSemanticBuiltin {
    typedef vector<string> Params;

    string format;

    VRSemanticBuiltin();
    virtual ~VRSemanticBuiltin();
    virtual bool execute(VRObjectPtr o, const Params& params) = 0;
};

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
    enum STATE {VALID, INVALID};
    STATE state = VALID; // valid, assumption, anonymous, ruled_out, ...
};

struct Variable {
    map<int, VREntityPtr> entities;
    map<int, Evaluation> evaluations; // summarize the evaluation state of each instance!
    string value;
    string concept = "var";
    bool isAssumption = false;
    bool isAnonymous = true;
    bool valid = true;

    string toString();
    bool has(VariablePtr other, VPath& p1, VPath& p2, VROntologyPtr onto);
    bool is(VariablePtr other, VPath& p1, VPath& p2);

    Variable(VROntologyPtr onto, string concept, string var);
    Variable(VROntologyPtr onto, string val);
    Variable();

    static VariablePtr create(VROntologyPtr onto, string concept, string var);
    static VariablePtr create(VROntologyPtr onto, string val);

    void addEntity(VREntityPtr e);
    bool operator==(Variable v);
    void discard(VREntityPtr e);
};

struct Term {
    VPath path;
    VariablePtr var;
    string str;

    Term(string s);
    bool valid();

    bool isMathExpression();
    string computeExpression(VRSemanticContextPtr c);

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

struct VRSemanticContext {
    map<string, VariablePtr> vars;
    map<string, Query> rules;
    list<Query> queries;
    vector<VREntityPtr> results;
    VROntologyPtr onto = 0;

    int itr=0;
    int itr_max = 5;

    VRSemanticContext(VROntologyPtr onto = 0);

    static VRSemanticContextPtr create(VROntologyPtr onto = 0);
};

OSG_END_NAMESPACE;

#endif // VRSEMANTICUTILS_H_INCLUDED
