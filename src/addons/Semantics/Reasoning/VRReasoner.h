#ifndef VRREASONER_H_INCLUDED
#define VRREASONER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>
#include <vector>
#include <map>
#include <list>

using namespace std;

#include "VROntology.h"

struct Path {
    string first;
    string root;
    vector<string> nodes;

    Path(string p);
    string toString();
};

struct Variable {
    vector<VREntity*> instances;
    string value;
    string concept;
    bool isAnonymous = false;
    bool valid = false;

    Variable();

    string toString();
    bool has(Variable& other, VROntology* onto);

    Variable(VROntology* onto, string concept, string var);
    Variable(VROntology* onto, string val);
    bool operator==(Variable v);
};

struct Result {
    vector<VREntity*> instances;
};

struct Term {
    Path path;
    Variable var;
    string str;

    Term(string s);
    bool valid();
    bool operator==(Term& other);
};

struct Statement {
    string verb;
    vector<Term> terms;
    int state = 0;

    Statement();
    Statement(string s);

    string toString();
    void updateLocalVariables(map<string, Variable>& globals, VROntology* onto);
    bool isSimpleVerb();
    bool match(Statement s);
};

struct Query {
    Statement request;
    vector<Statement> statements;

    Query(string q);
    string toString();
};

struct Context {
    map<string, Variable> vars;
    map<string, Result> results;
    list<Query> queries;
    VROntology* onto = 0;

    int itr=0;
    int itr_max = 5;

    Context(VROntology* onto);
    Context();
};

class VRReasoner {
    public:
        string pre = "  ?!?  ";

        static vector<string> split(string s, string d);
        static vector<string> split(string s, char d);
        static bool startswith(string s, string subs);

    private:
        VRReasoner();

        bool evaluate(Statement& s, Context& c);
        bool is(Statement& s, Context& c);
        bool has(Statement& s, Context& c);
        void pushQuery(Statement& s, Context& c);

    public:
        static VRReasoner* get();
        vector<Result> process(string query, VROntology* onto);
};


#endif // VRREASONER_H_INCLUDED
