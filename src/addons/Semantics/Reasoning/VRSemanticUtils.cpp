#include "VRSemanticUtils.h"
#include "VREntity.h"
#include "VROntology.h"
#include "VRReasoner.h"
#include "VRProperty.h"

#include <iostream>

using namespace OSG;

Variable::Variable() {;}

string Variable::toString() {
    string s = value+"("+concept+"){";
    for (auto i : instances) s += i.second->getName()+",";
    if (instances.size() > 0) s.pop_back();
    s +="}[";
    if (isAnonymous) s += "anonymous, ";
    if (isAssumption) s += "assumption, ";
    s += valid ? "valid" : "invalid";
    s +="]";
    return s;
}

/* Variable flags:
    isAssumption: no instances of the type have been found
    isAnonymous: no instances with that name have been found
    !valid: something is terribly wrong!
*/

Variable::Variable(VROntologyPtr onto, string concept, string var) {
    auto cl = onto->getConcept(concept);
    if (cl == 0) return;

    if ( auto i = onto->getInstance(var) ) {
        instances[i->ID] = i; // found an instance with the right name
        this->concept = i->getConcept()->getName();
        isAnonymous = false;
    } else { // get all instances of the required type
            for (auto i : onto->getInstances(concept)) instances[i->ID] = i;
        if (instances.size() == 0) {
            auto i = onto->addInstance(var, concept);
            instances[i->ID] = i;
            isAssumption = true;
        }
        this->concept = concept;
    }

    value = var;
    valid = true;
}

Variable::Variable(VROntologyPtr onto, string val) {
    value = val;
    concept = "var";
    valid = true;
}

shared_ptr<Variable> Variable::create(VROntologyPtr onto, string concept, string var) { return shared_ptr<Variable>( new Variable(onto, concept, var) ); }
shared_ptr<Variable> Variable::create(VROntologyPtr onto, string val) { return shared_ptr<Variable>( new Variable(onto, val) ); }

bool Variable::operator==(Variable v) {
    if (!v.valid || !valid) return false;
    for (uint i=0; i<instances.size(); i++) {
        if (v.instances[i] != instances[i]) return false;
    }
    return true;
}

void Variable::discard(VREntityPtr e) {
    if (!instances.count(e->ID)) return;
    instances.erase(e->ID);
}

VPath::VPath(string p) {
    nodes = VRReasoner::split(p, '/');
    root = nodes[0];
    first = nodes[nodes.size()-1];
}

string VPath::toString() {
    string s;
    for (auto p : nodes) s += p + "/";
    if (nodes.size() > 0) s.pop_back();
    return s;
}

Context::Context() {}

Context::Context(VROntologyPtr onto) {
    this->onto = onto;

    cout << "Init context:" << endl;
    for (auto i : onto->instances) {
        if (!i.second->getConcept()) { cout << "Context::Context instance " << i.second->getName() << " has no concept!" << endl; continue; }
        vars[i.second->getName()] = Variable::create( onto, i.second->getConcept()->getName(), i.second->getName() );
        cout << " add instance " << i.second->getName() << " of concept " << i.second->getConcept()->getName() << endl;
    }

    for ( auto r : onto->getRules()) {
        cout << "Context prep rule " << r->toString() << endl;
        Query q(r->toString());
        if (!q.request) continue;

        for (Term& t : q.request->terms) {
            t.var = Variable::create(onto,t.path.root);

            for (auto& s : q.statements) {
                if (s->isSimpleVerb()) continue;
                s->terms[0].var = Variable::create(onto, s->terms[0].path.root);
                auto var = s->terms[0].var;
                if (var->value != t.var->value) continue;

                t.var->concept = s->verb;
                cout << " Set concept of " << t.var->value << " to " << s->verb << endl;
                break;
            }
        }

        rules[r->toString()] = q;
    }
}

// TODO: parse concept statements here
Query::Query() {}

Query::Query(string q) {
    vector<string> parts = VRReasoner::split(q, ':');
    if (parts.size() > 0) request = VRStatement::create(parts[0]);
    if (parts.size() > 1) {
        parts = VRReasoner::split(parts[1], ';');
        for (int i=0; i<parts.size(); i++) statements.push_back(VRStatement::create(parts[i], i));
    }
}

string Query::toString() {
    string res = request->toString() + " :";
    for (auto s : statements) res += " " + s->toString();
    return res;
}

Term::Term(string s) : path(s), str(s) {;}

bool Term::valid() { return var->valid; }

bool Term::operator==(Term& other) {
    if (!valid() || !other.valid()) return false;

    for (auto i1 : var->instances) {
        vector<string> val1 = i1.second->getAtPath(path.nodes);
        for (auto i2 : other.var->instances) {
            vector<string> val2 = i2.second->getAtPath(other.path.nodes);
            for (string s2 : val1) {
                for (string s3 : val2) {
                    if (s2 == s3) return true;
                }
            }
        }
        for (string s : val1) if (s == other.var->value) return true;
    }
    return false;
}

bool Variable::has(VariablePtr other, VROntologyPtr onto) {
    for (auto i1 : instances) { // all instances of that variable
        vector<VREntityPtr> matches;
        vector<VREntityPtr> toDiscard;
        for (auto i2 : other->instances) { // check each instance of the other variable

            bool res = false;
            for (auto p : i1.second->properties) { // all properties of each instance
                for (auto v : p.second) {
                    if (v->value == other->value) return true; // TODO: direct match with other variable value
                    if (v->value == i2.second->getName()) { res = true; matches.push_back(i2.second); }
                    if (res) break;
                }
                if (res) break;
            }
            if (!res) toDiscard.push_back(i2.second);
        }

        for (auto e : toDiscard) other->discard(e);
        if (matches.size() > 0) return true;
    }

    return false;
}

void Query::checkState() {
    int r = 1;
    for (auto i : statements) if(i->state == 0) r = 0;
    request->state = r;
}
