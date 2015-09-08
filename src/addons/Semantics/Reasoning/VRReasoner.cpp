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

Variable::Variable() {;}

string Variable::toString() {
    string s = value+"("+concept+"){";
    for (auto i : instances) s += i->name+",";
    if (instances.size() > 0) s.pop_back();
    s +="}[";
    if (isAnonymous) s += "anonymous, ";
    s += valid ? "valid" : "invalid";
    s +="]";
    return s;
}

Variable::Variable(VROntology* onto, string concept, string var) {
    auto cl = onto->getConcept(concept);
    if (cl == 0) return;
    instances = onto->getInstances(concept);
    if (instances.size() == 0) {
        instances.push_back( onto->addInstance(var, concept) );
        isAnonymous = true;
    }
    value = var;
    this->concept = concept;
    valid = true;
}

Variable::Variable(VROntology* onto, string val) {
    value = val;
    concept = "var";
    valid = true;
}

bool Variable::operator==(Variable v) {
    if (!v.valid || !valid) return false;
    for (uint i=0; i<instances.size(); i++) {
        if (v.instances[i] != instances[i]) return false;
    }
    return true;
}

Path::Path(string p) {
    path = VRReasoner::split(p, '/');
    root = path[0];
    var = path[path.size()-1];
}

string Path::toString() {
    string s;
    for (auto p : path) s += p + "/";
    if (path.size() > 0) s.pop_back();
    return s;
}

Statement::Statement() {;}

Statement::Statement(string s) {
    auto s1 = VRReasoner::split(s, '(');
    verb = s1[0];
    auto s2 = VRReasoner::split( VRReasoner::split(s1[1], ')')[0] , ',');
    for (string s : s2) paths.push_back(Path(s));
}

string Statement::toString() {
    string s = verb + "(";
    for (auto p : paths) s += p.toString() + ",";
    if (paths.size() > 0) s.pop_back();
    s += ")";
    return s;
}

void Statement::updateLocalVariables(VRContext& context) {
    lvars.clear();
    for (auto p : paths) {
        Variable v;
        if (context.vars.count(p.root)) v = context.vars[p.root];
        else v = Variable(context.onto,p.root);
        lvars.push_back(v);
    }
}

bool Statement::isSimpleVerb() {
    static string verbs[] = {"q", "is", "is_all", "is_not", "has"};
    for (string v : verbs) if (v == verb) return true;
    return false;
}

bool Statement::match(Statement s) {
    for (uint i=0; i<lvars.size(); i++) {
        auto vS = s.lvars[i];
        auto vR = lvars[i];
        if (!vS.valid || !vR.valid) return false;
        if (!(vS == vR)) return false;
    }
    return true;
}

Query::Query(string q) {
    vector<string> parts = VRReasoner::split(q, ':');
    query = Statement(parts[0]);
    parts = VRReasoner::split(parts[1], ';');
    for (auto p : parts) statements.push_back(Statement(p));
}

string Query::toString() {
    return query.toString();
}

bool IS(Variable& v0, Variable& v1, Path& p0, Path& p1) {
    if (!v0.valid || !v1.valid) return false;
    for (auto i0 : v0.instances) {
        vector<string> val0 = i0->getAtPath(p0.path);
        for (auto i1 : v1.instances) {
            vector<string> val1 = i1->getAtPath(p1.path);
            for (string s1 : val0) {
                for (string s2 : val1) {
                    if (s1 == s2) return true;
                }
            }
        }
        for (string s1 : val0) if (s1 == v1.value) return true;
    }
    return false;
}

bool VRReasoner::evaluate(Statement& s, VRContext& c, list<Query>& queries) {
    cout << pre << " eval " << s.toString() << endl;
    if (s.state == 1) return true;
    s.updateLocalVariables(c);

    if (!s.isSimpleVerb()) { // resolve anonymous variables
        string var = s.paths[0].root;
        c.vars[var] = Variable( c.onto, s.verb, var );
        cout << pre << "  added variable " << c.vars[var].toString() << endl;
        return true;
    }

    if (s.verb == "is") {
        if (c.vars.count(s.lvars[0].value) == 0) return false; // check if context has a variable with the left value
        if (!s.lvars[0].valid || !s.lvars[1].valid) return false; // return if one of the sides invalid
        bool is = IS(s.lvars[0],s.lvars[1],s.paths[0],s.paths[1]);
        cout << pre << s.lvars[0].value << " is " << (is?"":" not ") << s.lvars[1].value << endl;
        if (is) { s.state = 1; return true; }

        for ( auto r : c.onto->getRules()) { // no match found -> check rules and initiate new queries
            Query R(r->rule);
            if (R.query.verb != s.verb) continue;
            R.query.updateLocalVariables(c);
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

VRContext::VRContext() { ; }
VRContext::VRContext(VROntology* onto) {
    this->onto = onto;
}

    // TODO: introduce requirements rules for the existence of some individuals

vector<Result> VRReasoner::process(string query, VROntology* onto) {
    cout << pre << query << endl;

    VRContext c(onto); // create context

    list<Query> queries;
    queries.push_back(Query(query));

    for(; queries.size() > 0 && c.itr < c.itr_max; c.itr++) { // while queries to process
        Query q = queries.back();
        if (q.query.state == 1) { queries.pop_back(); continue; }; // query answered, pop and continue

        cout << pre << "query " << q.toString() << endl;

        q.query.updateLocalVariables(c);

        if ( evaluate(q.query, c, queries) ) {
            if (q.query.verb == "q") {
                string v = q.query.lvars[0].value;
                if (c.results.count(v) == 0) c.results[v] = Result();
                c.results[v].instances = c.vars[v].instances;
            }
        }

        for (auto statement : q.statements) evaluate(statement, c, queries);
    }


    cout << pre << " break after " << c.itr << " queries\n";
    for (auto r : c.results) {
        cout << pre << "  result " << r.first << endl;
        for (auto i : r.second.instances) cout << pre << "   instance " << i->toString() << endl;
    }

    vector<Result> res;
    for (auto r : c.results) res.push_back(r.second);
    return res;
}
