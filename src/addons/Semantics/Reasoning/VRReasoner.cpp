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
    nodes = VRReasoner::split(p, '/');
    root = nodes[0];
    first = nodes[nodes.size()-1];
}

string Path::toString() {
    string s;
    for (auto p : nodes) s += p + "/";
    if (nodes.size() > 0) s.pop_back();
    return s;
}

Statement::Statement() {;}

Statement::Statement(string s) {
    auto s1 = VRReasoner::split(s, '(');
    verb = s1[0];
    auto s2 = VRReasoner::split( VRReasoner::split(s1[1], ')')[0] , ',');
    for (string s : s2) {
        terms.push_back(Term(s));
    }
}

string Statement::toString() {
    string s = verb + "(";
    for (auto t : terms) s += t.path.toString() + ",";
    if (terms.size() > 0) s.pop_back();
    s += ")";
    return s;
}

void Statement::updateLocalVariables(VRContext& context) {
    for (auto& t : terms) {
        if (context.vars.count(t.path.root)) t.var = context.vars[t.path.root];
        else t.var = Variable(context.onto,t.path.root);
    }
}

bool Statement::isSimpleVerb() {
    static string verbs[] = {"q", "is", "is_all", "is_not", "has"};
    for (string v : verbs) if (v == verb) return true;
    return false;
}

bool Statement::match(Statement s) {
    for (uint i=0; i<terms.size(); i++) {
        auto tS = s.terms[i];
        auto tR = terms[i];
        if (!tS.valid() || !tR.valid()) return false;
        if (!(tS == tR)) return false;
    }
    return true;
}

Query::Query(string q) {
    vector<string> parts = VRReasoner::split(q, ':');
    request = Statement(parts[0]);
    parts = VRReasoner::split(parts[1], ';');
    for (auto p : parts) statements.push_back(Statement(p));
}

string Query::toString() {
    return request.toString();
}

Term::Term(string s) : path(s), str(s) {;}

bool Term::valid() { return var.valid; }

bool Term::operator==(Term& other) {
    if (!valid() || !other.valid()) return false;

    for (auto i1 : var.instances) {
        vector<string> val1 = i1->getAtPath(path.nodes);
        for (auto i2 : other.var.instances) {
            vector<string> val2 = i2->getAtPath(other.path.nodes);
            for (string s2 : val1) {
                for (string s3 : val2) {
                    if (s2 == s3) return true;
                }
            }
        }
        for (string s : val1) if (s == other.var.value) return true;
    }
    return false;
}

bool VRReasoner::is(Statement& statement, VRContext& context, list<Query>& queries) {
    auto left = statement.terms[0];
    auto right = statement.terms[1];

    if ( context.vars.count(left.var.value) == 0) return false; // check if context has a variable with the left value
    if (!left.valid() || !right.valid()) return false; // return if one of the sides invalid

    bool b = left == right;

    cout << pre << "   " << left.str << " is " << (b?"":" not ") << right.var.value << endl;

    if (b) { statement.state = 1; return true; }

    for ( auto r : context.onto->getRules()) { // no match found -> check rules and initiate new queries
        Query query(r->rule);
        if (query.request.verb != statement.verb) continue;
        query.request.updateLocalVariables(context);
        if (statement.match(query.request)) queries.push_back(query);
    }
    return false;
}

bool VRReasoner::has(Statement& s, VRContext& c, list<Query>& queries) { // TODO
    cout << pre << "  has not implemented, ignoring" << endl;
    return false;
}

bool VRReasoner::evaluate(Statement& s, VRContext& c, list<Query>& queries) {
    cout << pre << " eval " << s.toString() << endl;
    if (s.state == 1) return true;
    s.updateLocalVariables(c);

    if (!s.isSimpleVerb()) { // resolve anonymous variables
        string var = s.terms[0].path.root;
        c.vars[var] = Variable( c.onto, s.verb, var );
        cout << pre << "  added variable " << c.vars[var].toString() << endl;
        return true;
    }

    if (s.verb == "is") return is(s,c,queries);
    if (s.verb == "is_not") return !is(s,c,queries);
    if (s.verb == "has") return has(s,c,queries);
    if (s.verb == "has_not") return !has(s,c,queries);
    return false;
}

VRContext::VRContext() { ; }
VRContext::VRContext(VROntology* onto) {
    this->onto = onto;
}

    // TODO: introduce requirements rules for the existence of some individuals

vector<Result> VRReasoner::process(string initial_query, VROntology* onto) {
    cout << pre << initial_query << endl;

    VRContext context(onto); // create context

    list<Query> queries;
    queries.push_back(Query(initial_query));

    while( queries.size() ) { // while queries to process
        Query query = queries.back();
        auto request = query.request;
        if (request.state == 1) { queries.pop_back(); continue; }; // query answered, pop and continue

        cout << pre << "query " << query.toString() << endl;

        request.updateLocalVariables(context);

        if ( evaluate(request, context, queries) ) {
            if (request.verb == "q") {
                string v = request.terms[0].var.value;
                if (context.results.count(v) == 0) context.results[v] = Result();
                context.results[v].instances = context.vars[v].instances;
            }
        }

        for (auto statement : query.statements) evaluate(statement, context, queries);

        context.itr++;
        if (context.itr >= context.itr_max) break;
    }

    cout << pre << " break after " << context.itr << " queries\n";
    for (auto r : context.results) {
        cout << pre << "  result " << r.first << endl;
        for (auto i : r.second.instances) cout << pre << "   instance " << i->toString() << endl;
    }

    vector<Result> res;
    for (auto r : context.results) res.push_back(r.second);
    return res;
}
