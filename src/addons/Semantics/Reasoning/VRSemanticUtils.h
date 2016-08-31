#ifndef VRSEMANTICUTILS_H_INCLUDED
#define VRSEMANTICUTILS_H_INCLUDED

#include <string>
#include <vector>
#include <map>
#include <list>
#include <OpenSG/OSGConfig.h>

#include "../VRSemanticsFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

struct VPath {
    string first;
    string root;
    vector<string> nodes;

    VPath(string p);
    string toString();
};

struct Variable {
    map<int, VREntityPtr> instances;
    string value;
    string concept = "var";
    bool isAssumption = false;
    bool isAnonymous = true;
    bool valid = true;

    Variable();

    string toString();
    bool has(std::shared_ptr<Variable> other, VROntologyPtr onto);

    Variable(VROntologyPtr onto, string concept, string var);
    Variable(VROntologyPtr onto, string val);

    static std::shared_ptr<Variable> create(VROntologyPtr onto, string concept, string var);
    static std::shared_ptr<Variable> create(VROntologyPtr onto, string val);

    bool operator==(Variable v);
    void discard(VREntityPtr e);
};

typedef std::shared_ptr<Variable> VariablePtr;
typedef std::weak_ptr<Variable> VariableWeakPtr;

struct Result {
    vector<VREntityPtr> instances;
};

struct Term {
    VPath path;
    VariablePtr var;
    string str;

    Term(string s);
    bool valid();
    bool operator==(Term& other);
};

struct Query {
    VRStatementPtr request;
    vector<VRStatementPtr> statements;

    Query();
    Query(string q);
    string toString();

    void checkState();
};

struct Context {
    map<string, VariablePtr> vars;
    map<string, Result> results;
    map<string, Query> rules;
    list<Query> queries;
    VROntologyPtr onto = 0;

    int itr=0;
    int itr_max = 5;

    Context(VROntologyPtr onto);
    Context();
};

OSG_END_NAMESPACE;

#endif // VRSEMANTICUTILS_H_INCLUDED
