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
    string root;
    string var;
    vector<string> path;

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

    Variable(VROntology* onto, string concept, string var);
    Variable(VROntology* onto, string val);
    bool operator==(Variable v);
};

struct Result {
    vector<VREntity*> instances;
};

struct VRContext {
    map<string, Variable> vars;
    map<string, Result> results;
    VROntology* onto = 0;

    int itr=0;
    int itr_max = 20;

    VRContext(VROntology* onto);
    VRContext();
};

struct Statement {
    string verb;
    vector<Path> paths;
    vector<Variable> lvars;
    int state = 0;

    Statement();

    Statement(string s);

    string toString();

    void updateLocalVariables(VRContext& context);

    bool isSimpleVerb();

    bool match(Statement s);
};

struct Query {
    Statement query;
    vector<Statement> statements;

    Query(string q);
    string toString();
};

class VRReasoner {
    public:
        string pre = "  ?!?  ";

        static vector<string> split(string s, string d);
        static vector<string> split(string s, char d);
        static bool startswith(string s, string subs);

    private:
        VRReasoner();

        bool evaluate(Statement& s, VRContext& c, list<Query>& queries);

    public:
        static VRReasoner* get();
        vector<Result> process(string query, VROntology* onto);
};


#endif // VRREASONER_H_INCLUDED
