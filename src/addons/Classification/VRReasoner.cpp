#include "VRReasoner.h"
#include "VROntology.h"

#include <iostream>
#include <sstream>
#include <list>
#include "core/utils/toString.h"

using namespace std;

VRReasoner::VRReasoner() {;}

VRReasoner* VRReasoner::get() {
    static VRReasoner* r = new VRReasoner();
    return r;
}

vector<string> VRReasoner::split(string s, string d) {
    vector<string> res;
    size_t pos = 0;
    int dN = d.length();

    string t;
    while ((pos = s.find(d)) != string::npos) {
        t = s.substr(0, pos);
        s.erase(0, pos + dN);
        res.push_back(t);
    }

    return res;
}

vector<string> VRReasoner::split(string s, char d) {
    vector<string> res;
    string t;
    stringstream ss(s);
    while (getline(ss, t, d)) res.push_back(t);
    return res;
}

bool VRReasoner::startswith(string s, string subs) {
    return s.compare(0, subs.size(), subs);
}

struct Variable {
    vector<VROntologyInstance*> instances;
    string value;
    bool isAnonymous = false;
    bool valid = false;

    Variable() {;}

    string toString() {
        string s = value+"[";
        for (auto i : instances) s += i->name+",";
        if (instances.size() > 0) s.pop_back();
        s +="]{"+::toString(isAnonymous)+","+::toString(valid)+"}";
        return s;
    }

    Variable(VROntology* onto, string concept, string var) {
        auto cl = onto->getConcept(concept);
        if (cl == 0) return;
        instances = onto->getInstances(concept);
        if (instances.size() == 0) {
            instances.push_back( onto->addInstance(var, concept) );
            isAnonymous = true;
        }
        value = var;
        valid = true;
    }

    Variable(VROntology* onto, string val) {
        value = val;
        valid = true;
    }

    bool operator==(Variable v) {
        if (!v.valid || !valid) return false;
        for (int i=0; i<instances.size(); i++) {
            if (v.instances[i] != instances[i]) return false;
        }
        return true;
    }
};

struct Path {
    string root;
    string var;
    vector<string> path;
    Path(string p) {
        path = VRReasoner::split(p, '/');
        root = path[0];
        var = path[path.size()-1];
    }

    string toString() {
        string s;
        for (auto p : path) s += p + "/";
        if (path.size() > 0) s.pop_back();
        return s;
    }
};

struct Statement {
    string verb;
    vector<Path> paths;
    vector<Variable> lvars;
    int state = 0;

    Statement() {;}

    Statement(string s) {
        auto s1 = VRReasoner::split(s, '(');
        verb = s1[0];
        auto s2 = VRReasoner::split( VRReasoner::split(s1[1], ')')[0] , ',');
        for (string s : s2) paths.push_back(Path(s));
    }

    string toString() {
        string s = verb + "(";
        for (auto p : paths) s += p.toString() + ",";
        if (paths.size() > 0) s.pop_back();
        s += ")";
        return s;
    }

    void updateLocalVariables(map<string, Variable>& globals, VROntology* onto) {
        lvars.clear();
        for (auto p : paths) {
            Variable v;
            if (globals.count(p.root)) v = globals[p.root];
            else v = Variable(onto,p.root);
            lvars.push_back(v);
        }
    }

    bool isSimpleVerb() {
        static string verbs[] = {"q", "is", "is_all", "is_not", "has"};
        for (string v : verbs) if (v == verb) return true;
        return false;
    }

    bool match(Statement s) {
        for (int i=0; i<lvars.size(); i++) {
            auto vS = s.lvars[i];
            auto vR = lvars[i];
            if (!vS.valid || !vR.valid) return false;
            if (!(vS == vR)) return false;
        }
        return true;
    }
};

struct Query {
    Statement query;
    vector<Statement> statements;
    Query(string q) {
        vector<string> parts = VRReasoner::split(q, ':');
        query = Statement(parts[0]);
        parts = VRReasoner::split(parts[1], ';');
        for (auto p : parts) statements.push_back(Statement(p));
    }

    string toString() {
        return query.toString();
    }
};

bool IS(Variable& v0, Variable& v1, Path& p0, Path& p1) {
    if (!v0.valid || !v1.valid) return false;
    for (auto i0 : v0.instances) {
        vector<string> val0 = i0->getAtPath(p0.path);
        for (auto i1 : v1.instances) {
            vector<string> val1 = i1->getAtPath(p1.path);
            for (string s1 : val0) {
                for (string s2 : val1) if (s1 == s2) return true;
            }
        }
        for (string s1 : val0) if (s1 == v1.value) return true;
    }
    return false;
}

bool Eval(Statement& s, map<string, Variable>& vars, VROntology* onto, list<Query>& queries) {
    cout << " eval " << s.toString() << endl;
    if (s.state == 1) return true;
    s.updateLocalVariables(vars, onto);

    if (!s.isSimpleVerb()) { // resolve anonymous variables
        string var = s.paths[0].root;
        vars[var] = Variable( onto, s.verb, var );
        cout << "  added variable " << vars[var].toString() << endl;
        return true;
    }

    if (s.verb == "is") {
        if (vars.count(s.lvars[0].value) == 0) return false;
        if (!s.lvars[0].valid || !s.lvars[1].valid) return false;
        if (IS(s.lvars[0],s.lvars[1],s.paths[0],s.paths[1])) { s.state = 1; return true; }

        for ( auto r : onto->getRules()) { // no match found -> check rules and initiate new queries
            Query R(r->rule);
            if (R.query.verb != s.verb) continue;
            R.query.updateLocalVariables(vars, onto);
            if (s.match(R.query)) queries.push_back(R);
        }
        return false;
    }

    if (s.verb == "is_not") {
        return false;
    }

    if (s.verb == "has") {
        //if (vars.count(var[0]) == 0) continue;
        //if (vars.count(var[1]) == 0) continue;
        return false;
    }
    return false;
}

    // TODO: introduce requirements rules for the existence of some individuals

vector<VRReasoner::Result> VRReasoner::process(string query, VROntology* onto) {
    cout << "VRReasoner query: " << query << endl;

    map<string, Variable> vars;
    map<string, Result> results;
    list<Query> queries;
    queries.push_back(Query(query));

    int itr=0;
    int itr_max = 20;

    for(; queries.size() > 0 && itr < itr_max; itr++) {
        Query q = queries.back();
        cout << "query " << q.toString() << endl;

        if (q.query.state == 1) { queries.pop_back(); continue; };
        q.query.updateLocalVariables(vars, onto);

        if (Eval(q.query, vars, onto, queries)) {
            if (q.query.verb == "q") {
                string v = q.query.lvars[0].value;
                if (results.count(v) == 0) results[v] = Result();
                results[v].instances = vars[v].instances;
            }
        }

        for (auto s : q.statements) Eval(s, vars, onto, queries);
    }


    cout << " break after " << itr << " queries\n";
    for (auto r : results) {
        cout << "  result " << r.first << endl;
        for (auto i : r.second.instances) cout << "   instance " << i->toString() << endl;
    }

    vector<VRReasoner::Result> res;
    for (auto r : results) res.push_back(r.second);
    return res;
}
