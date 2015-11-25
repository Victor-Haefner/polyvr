#ifndef VRREASONER_H_INCLUDED
#define VRREASONER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>
#include <vector>
#include <map>
#include <list>
#include <memory>

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
    vector<VREntityPtr> instances;
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
    vector<VREntityPtr> instances;
};

struct Term {
    Path path;
    Variable var;
    string str;

    Term(string s);
    bool valid();
    bool operator==(Term& other);
};

struct Statement;
typedef shared_ptr<Statement> StatementPtr;
typedef weak_ptr<Statement> StatementWeakPtr;

struct Statement {
    string verb;
    string verb_suffix;
    vector<Term> terms;
    int state = 0;
    int place = -1;

    Statement();
    Statement(string s, int i = -1);
    static StatementPtr New(string s, int i = -1);

    string toString();
    void updateLocalVariables(map<string, Variable>& globals, VROntology* onto);
    bool isSimpleVerb();
    bool match(StatementPtr s);
};

struct Query {
    StatementPtr request;
    vector<StatementPtr> statements;

    Query();
    Query(string q);
    string toString();

    void checkState();
};

struct Context {
    map<string, Variable> vars;
    map<string, Result> results;
    map<string, Query> rules;
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
        string redBeg  = "\033[0;38;2;255;150;150m";
        string greenBeg = "\033[0;38;2;150;255;150m";
        string blueBeg = "\033[0;38;2;150;150;255m";
        string yellowBeg = "\033[0;38;2;255;255;150m";
        string colEnd = "\033[0m";

        static vector<string> split(string s, string d);
        static vector<string> split(string s, char d);
        static bool startswith(string s, string subs);

    private:
        VRReasoner();

        bool evaluate(StatementPtr s, Context& c);
        bool apply(StatementPtr s, Context& c);
        bool is(StatementPtr s, Context& c);
        bool has(StatementPtr s, Context& c);
        bool findRule(StatementPtr s, Context& c);

    public:
        static VRReasoner* get();
        vector<Result> process(string query, VROntology* onto);
};


#endif // VRREASONER_H_INCLUDED
