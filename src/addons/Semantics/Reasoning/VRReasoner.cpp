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

Context::Context() {}

Context::Context(VROntology* onto) {
    this->onto = onto;

    cout << "Init context:" << endl;
    for (auto i : onto->instances) {
        vars[i.second->name] = Variable( onto, i.second->concept->name, i.second->name );
        cout << " add instance " << i.second->name << " of concept " << i.second->concept->name << endl;
    }
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

void Statement::updateLocalVariables(map<string, Variable>& globals, VROntology* onto) {
    for (auto& t : terms) {
        if (globals.count(t.path.root)) t.var = globals[t.path.root];
        else t.var = Variable(onto,t.path.root);
        //cout << "updateLocalVariables " << t.str << " " << t.var.value << " " << t.var.concept << endl;
    }
}

bool Statement::isSimpleVerb() {
    static string verbs[] = {"q", "is", "is_all", "is_not", "has", "has_not"};
    for (string v : verbs) if (v == verb) return true;
    return false;
}

bool Statement::match(Statement& s) {
    for (uint i=0; i<terms.size(); i++) {
        auto tS = s.terms[i];
        auto tR = terms[i];
        if (!tS.valid() || !tR.valid()) return false;
        if (tS.path.nodes.size() != tR.path.nodes.size()) return false;

        cout << "match " << tR.var.value << " " << tS.var.value << endl;
        cout << "match " << tR.var.concept << " " << tS.var.concept << endl;
        if (tR.var.concept != tS.var.concept) return false;
    }
    return true;
}

// TODO: parse concept statements here
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

void VRReasoner::pushQuery(Statement& statement, Context& context) {

    cout << pre << "     search rule for " << statement.toString() << endl;
    for ( auto r : context.onto->getRules()) { // no match found -> check rules and initiate new queries
        Query query(r->rule);
        if (query.request.verb != statement.verb) continue; // rule verb does not match
        cout << pre << "      rule " << query.request.toString() << endl;

        query.request.updateLocalVariables(context.vars, context.onto);
        if (!statement.match(query.request)) continue; // statements are not similar enough



        context.queries.push_back(query);
        cout << pre << "      add query " << query.toString() << endl;
        return;
    }
}

bool VRReasoner::is(Statement& statement, Context& context) {
    auto left = statement.terms[0];
    auto right = statement.terms[1];

    if ( context.vars.count(left.var.value) == 0) return false; // check if context has a variable with the left value
    if (!left.valid() || !right.valid()) return false; // return if one of the sides invalid

    bool b = left == right;

    cout << pre << "   " << left.str << " is " << (b?"":" not ") << right.var.value << endl;

    if (b) { statement.state = 1; return true; }

    pushQuery(statement, context);
    return false;
}

bool Variable::has(Variable& other, VROntology* onto) {
    for (auto i1 : instances) {
        for (auto p : i1->properties) { // check if instance properties have
            for (auto v : p.second) { // TODO: compare each v with other.value
                if (v == other.value) return true;
            }
        }
    }
    return false;
}

bool VRReasoner::has(Statement& statement, Context& context) { // TODO
    auto left = statement.terms[0];
    auto right = statement.terms[1];

    bool b = left.var.has(right.var, context.onto);
    cout << pre << "  " << left.str << " has " << (b?"":"not") << " " << right.str << endl;
    if (b) { statement.state = 1; return true; }

    auto Cconcept = context.onto->getConcept( right.var.concept ); // child concept
    auto Pconcept = context.onto->getConcept( left.var.concept ); // parent concept
    if (Cconcept == 0) { cout << "       Warning! concept " << right.var.concept << " not found!\n"; return false; }
    if (Pconcept == 0) { cout << "       Warning! concept " << left.var.concept << " not found!\n"; return false; }

    auto prop = Pconcept->getProperty( Cconcept->name );
    if (prop == 0) { // no property
        cout << "       Warning! concept " << Pconcept->name << " has no proprty of type " << Cconcept->name << "!\n";
        return false;
    }

    for (auto i : left.var.instances) {
        if (i->properties.count(prop->ID) == 0) {
            i->properties[prop->ID] = vector<string>();
        }

        i->properties[prop->ID].push_back(right.var.value);
    }

    pushQuery(statement, context);
    return false;
}

bool VRReasoner::evaluate(Statement& statement, Context& context) {
    statement.updateLocalVariables(context.vars, context.onto);

    if (!statement.isSimpleVerb()) { // resolve anonymous variables
        string var = statement.terms[0].path.root;
        context.vars[var] = Variable( context.onto, statement.verb, var );
        cout << pre << "  added variable " << context.vars[var].toString() << endl;
        statement.state = 1;
        return true;
    }

    if (statement.verb == "is") return is(statement,context);
    if (statement.verb == "is_not") return !is(statement,context); // TODO, is wrong
    if (statement.verb == "has") return has(statement,context);
    if (statement.verb == "has_not") return !has(statement,context); // TODO, is wrong
    return false;
}

    // TODO: introduce requirements rules for the existence of some individuals

vector<Result> VRReasoner::process(string initial_query, VROntology* onto) {
    cout << pre << initial_query << endl;

    Context context(onto); // create context
    context.queries.push_back(Query(initial_query));

    while( context.queries.size() ) { // while queries to process
        Query& query = context.queries.back();
        auto request = query.request;
        if (request.state == 1) { context.queries.pop_back(); continue; }; // query answered, pop and continue

        cout << pre << "QUERY " << query.toString() << endl;

        request.updateLocalVariables(context.vars, context.onto);

        if ( evaluate(request, context) ) {
            if (request.verb == "q") {
                string v = request.terms[0].var.value;
                if (context.results.count(v) == 0) context.results[v] = Result();
                context.results[v].instances = context.vars[v].instances;
            }
        }

        int i=0;
        for (auto& statement : query.statements) {
            i++;
            if (statement.state == 1) continue;
            cout << pre << " " << i << " eval " << statement.toString() << endl;
            if (evaluate(statement, context)) statement.state = 1;
        }

        context.itr++;
        if (context.itr >= context.itr_max) break;
    }

    cout << pre << " break after " << context.itr << " iterations\n";
    for (auto r : context.results) {
        cout << pre << "  result " << r.first << endl;
        for (auto i : r.second.instances) cout << pre << "   instance " << i->toString() << endl;
    }

    vector<Result> res;
    for (auto r : context.results) res.push_back(r.second);
    return res;
}
