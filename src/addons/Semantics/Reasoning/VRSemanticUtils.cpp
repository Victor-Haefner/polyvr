#include "VRSemanticUtils.h"
#include "VREntity.h"
#include "VROntology.h"
#include "VRReasoner.h"
#include "VRProperty.h"
#include "core/utils/toString.h"
#include "core/utils/VRFunction.h"
#include "core/math/Expression.h"

#include <stack>
#include <iostream>
#include <algorithm>

using namespace OSG;

VRSemanticBuiltin::VRSemanticBuiltin() {}
VRSemanticBuiltin::~VRSemanticBuiltin() {}

Variable::Variable() {;}

string Variable::toString() {
    string s = value+"(" + concept + "){";
    for (auto i : entities) s += i.second->getName()+",";
    if (entities.size() > 0) s.pop_back();
    s +="}[";
    if (isAnonymous) s += "anonymous, ";
    if (isAssumption) s += "assumption, ";
    s += valid ? "valid" : "invalid";
    s +="]";
    return s;
}

/* Variable flags:
    isAssumption: no entities of the type have been found
    isAnonymous: no entities with that name have been found
    !valid: something is terribly wrong!
*/

Variable::Variable(VROntologyPtr onto, string concept, string var) {
    auto cl = onto->getConcept(concept);
    if (cl == 0) return;

    if ( auto i = onto->getEntity(var) ) {
        addEntity(i);
        this->concept = concept; // TODO: maybe the entity has a concept that inherits from the concept passed above?
        isAnonymous = false;
    } else { // get all entities of the required type
        for (auto i : onto->getEntities(concept)) addEntity(i);
        if (entities.size() == 0) {
            auto i = onto->addEntity(var, concept);
            addEntity(i);
            isAssumption = true; // TODO: put that in the evaluation
        }
        this->concept = concept;
    }

    value = var;
    valid = true;
}

Variable::Variable(VROntologyPtr onto, string val) {
    value = val;
}

shared_ptr<Variable> Variable::create(VROntologyPtr onto, string concept, string var) { return shared_ptr<Variable>( new Variable(onto, concept, var) ); }
shared_ptr<Variable> Variable::create(VROntologyPtr onto, string val) { return shared_ptr<Variable>( new Variable(onto, val) ); }

void Variable::addEntity(VREntityPtr e) {
    entities[e->ID] = e;
    evaluations[e->ID] = Evaluation();
}

bool Variable::has(VariablePtr other, VPath& path1, VPath& path2, VROntologyPtr onto) {
    map<VREntityPtr, vector<VREntityPtr>> matches;

    map<VREntity*, bool> visited;

    function<bool(VREntityPtr, string&)> computeMatches = [&](VREntityPtr e, string& oName) -> bool {
        if (!e) return false;
        if (visited.count(e.get())) return false; // check for cycles / visited graph nodes
        visited[e.get()] = true;

        for (auto p : e->properties) { // property vectors of local entity
            for (auto v : p.second) { // local properties
                //if (v->value == other->value) matches[e].push_back(0); // TODO: direct match with other variable value
                if (v->value == oName) return true;
                //auto childEntity = onto->getEntity(v->value); // TODO: this might by stupid..
                //if (childEntity && computeMatches(childEntity, oName)) return true;
            }
        }
        return false;
    };

    // get all matches
    for (auto i2 : other->entities) { // check each instance of the other variable
        string oName = i2.second->getName();
        for (auto i1 : entities) { // all entities of that variable
            for (auto v : path1.getValue(i1.second)) {
                bool doMatch = computeMatches(onto->getEntity(v), oName);
                if (doMatch) matches[i1.second].push_back(i2.second);
            }
        }
        visited.clear();
    }

    // remove non matched entities
    vector<VREntityPtr> toDiscard1;
    vector<VREntityPtr> toDiscard2;
    for (auto i1 : entities) { // all entities of that variable
        if (matches.count(i1.second) == 0) toDiscard1.push_back(i1.second);
    }

    for (auto i2 : other->entities) { // check each entity of the other variable
        bool found = false;
        for (auto ev : matches) {
            for (auto e : ev.second) {
                found = (e == i2.second);
                if (found) break;
            }
            if (found) break;
        }
        if (!found) toDiscard2.push_back(i2.second);
    }

    for (auto e : toDiscard1) discard(e);
    for (auto e : toDiscard2) other->discard(e);

    return (matches.size() > 0);
}

bool Variable::is(VariablePtr other, VPath& path1, VPath& path2) {
    if (!valid || !other->valid) return false;

    auto hasSameVal = [&](vector<string>& val1, vector<string>& val2) {
        for (string s1 : val1) {
            for (string s2 : val2) if (s1 == s2) return true;
        }
        return false;
    };

    auto hasSameVal2 = [&](vector<string>& val1) {
        bool res = false;
        for (auto e : other->entities) {
            vector<string> val2 = path2.getValue(e.second);
            //for (auto v : val2) cout << " var2 value: " << v << endl;
            auto r = hasSameVal(val1, val2);
            if (!r) evaluations[e.first].state = Evaluation::INVALID;
            if (r) res = true;
        }
        if (res) return true;

        for (string s : val1) if (s == other->value) return true;
        return false;
    };

    //cout << "Variable::is " << toString() << " at path " << p1.toString() << " =?= " << other->toString() << " at path " << p2.toString() << endl;
    bool res = false;
    for (auto e : entities) {
        vector<string> val1 = path1.getValue(e.second);
        //for (auto v : val1) cout << " var1 value: " << v << endl;
        auto r = hasSameVal2(val1);
        if (!r) evaluations[e.first].state = Evaluation::INVALID;
        if (r) res = true;
    }
    return res;
}

bool Variable::operator==(Variable v) {
    if (!v.valid || !valid) return false;
    for (uint i=0; i<entities.size(); i++) {
        if (v.entities[i] != entities[i]) return false;
    }
    return true;
}

void Variable::discard(VREntityPtr e) {
    if (!entities.count(e->ID)) return;
    entities.erase(e->ID);
    evaluations.erase(e->ID);
}

VPath::VPath(string p) {
    nodes = VRReasoner::split(p, '.');
    if (nodes.size() == 0) return;
    root = nodes[0];
    first = nodes[nodes.size()-1];
}

string VPath::toString() {
    string s;
    for (auto p : nodes) s += p + '.';
    if (nodes.size() > 0) s.pop_back();
    return s;
}

int VPath::size() {
    return nodes.size();
}

vector<string> VPath::getValue(VREntityPtr e) {
    vector<string> res;
    if (!e) return res;
    auto onto = e->ontology.lock();
    if (!onto) return res;

    if (size() == 1) { res.push_back( e->getName() ); return res; }

    auto getEntityProperties = [&](VREntityPtr e, string m) {
        vector<VRPropertyPtr> res;
        auto prop = e->getProperty(m);
        if (!prop) return res;
        if (!e->properties.count(prop->getName())) return res;
        for (auto p : e->properties[prop->getName()]) res.push_back(p);
        return res;
    };

    if (size() == 2) {
        for (auto p : getEntityProperties(e,nodes[1])) res.push_back(p->value);
        return res;
    }

    if (size() == 3) {
        for (auto p : getEntityProperties(e,nodes[1])) {
            auto e2 = onto->getEntity(p->value);
            if (!e2) continue;
            for (auto p : getEntityProperties(e2,nodes[2])) res.push_back(p->value);
        }
        return res;
    }

    return res;
}

void VPath::setValue(string v, VREntityPtr e) {
    if (size() == 2) {
        string m = first;
        auto prop = e->getProperty(m);
        if (!prop) return;
        if (!e->properties.count(prop->getName())) return;
        for (auto p : e->properties[prop->getName()]) {
            p->value = v;
        }
    }
}

VRSemanticContext::VRSemanticContext(VROntologyPtr onto) {
    if (!onto) return;
    this->onto = onto;

    //cout << "Init VRSemanticContext:" << endl;
    for (auto i : onto->entities) {
        if (i.second->getConcepts().size() == 0) { cout << "VRSemanticContext::VRSemanticContext instance " << i.second->getName() << " has no concepts!" << endl; continue; }
        vars[i.second->getName()] = Variable::create( onto, i.second->getConcepts()[0]->getName(), i.second->getName() );
        //cout << " add instance " << i.second->toString() << endl;
    }

    for ( auto r : onto->getRules()) {
        //cout << "VRSemanticContext prep rule " << r->rule << endl;
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
                //cout << " Set concept of " << t.var->value << " to " << s->verb << endl;
                break;
            }
        }

        rules[r->rule] = q;
    }
}

VRSemanticContextPtr VRSemanticContext::create(VROntologyPtr onto) { return VRSemanticContextPtr( new VRSemanticContext(onto) ); }

// TODO: parse concept statements here
Query::Query() {}

Query::Query(string q) {
    vector<string> parts = VRReasoner::split(q, ':');
    if (parts.size() > 0) request = VRStatement::create(parts[0]);
    if (parts.size() > 1) {
        parts = VRReasoner::split(parts[1], ';');
        for (uint i=0; i<parts.size(); i++) statements.push_back(VRStatement::create(parts[i], i));
    }
}

string Query::toString() {
    string res = request->toString() + " :";
    for (auto s : statements) res += " " + s->toString();
    return res;
}

Term::Term(string s) : path(s), str(s) {}

bool Term::isMathExpression() { Expression e(str); return e.isMathExpression(); }

string Term::computeExpression(VRSemanticContextPtr context) {
    Expression me(str);
    if (!me.isMathExpression()) return "";
    me.computeTree(); // build RDP tree
    for (auto l : me.getLeafs()) {
        VPath p(l->param);
        l->setValue(p.root); // default is to use path root, might just be a number
        if (context->vars.count(p.root)) {
            auto v = context->vars[p.root];
            for (auto e : v->entities) {
                auto vals = p.getValue(e.second);
                for (auto val : vals) {
                    l->setValue(val);
                    cout << " computeExpression, replace " << p.root << " by " << val << endl;
                }
            }
        }
    }
    string res = me.compute();
    cout << " computeExpression '"+str+"' results to " << res << endl;
    return res;
}

bool Term::valid() { return var->valid; }

bool Term::is(Term& t, VRSemanticContextPtr context) {
    auto v = t.var;
    if (t.isMathExpression()) {
        auto res = t.computeExpression(context);
        v = Variable::create(0,res);
    }
    return var->is(v, path, t.path);
}

bool Term::has(Term& t, VRSemanticContextPtr context) {
    auto v = t.var;
    /*if (t.isMathExpression()) { // TODO: sure this does not apply?
        auto res = t.computeExpression(context);
        v = Variable::create(0,res);
    }*/
    return var->has(v, path, t.path, context->onto);
}

void Query::checkState() {
    int r = 1;
    for (auto i : statements) if(i->state == 0) r = 0;
    request->state = r;
}

void Query::substituteRequest(VRStatementPtr replace) { // replaces the roots of all paths of the terms of each statement
    // compute all values to substitute
    map<string, string> substitutes;
    for (int i=0; i<request->terms.size(); i++) {
        Term& t1 = request->terms[i];
        Term& t2 = replace->terms[i];
        substitutes[t1.var->value] = t2.str;
    }

    auto substitute = [&](string& var) {
        if (!substitutes.count(var)) return;
        cout << "  substitute: " << var;
        var = substitutes[var];
        cout << " with " << var << endl;
    };

    cout << " substitutes:\n";
    for (auto s : substitutes) cout << "  substitute "+s.first+" "+s.second << endl;

    for (auto statement : statements) { // substitute values in all statements of the query
        for (auto& ts : statement->terms) {
            if (ts.isMathExpression()) {
                Expression e(ts.str);
                e.computeTree();
                cout << " substitute expression: " << e.toString() << endl;
                for (auto& l : e.getLeafs()) {
                    for (int i=0; i<request->terms.size(); i++) {
                        auto& t1 = request->terms[i];
                        //cout << " substitute " << l->param << " , " << t1.path.root << " in expression " << ts.str << " ?" << endl;
                        if (t1.path.root == l->param) substitute(l->param);
                        else {
                            VPath lpath(l->param);
                            if (t1.path.root == lpath.root) {
                                substitute(lpath.root);
                                lpath.nodes[0] = lpath.root;
                                l->param = lpath.toString();
                            }
                        }
                    }
                }
                ts.str = e.toString();
                ts.path = VPath(ts.str);
                cout << " substituted expression: " << ts.str << endl;
            } else {
                for (int i=0; i<request->terms.size(); i++) {
                    auto& t1 = request->terms[i];
                    if (t1.path.root == ts.path.root) {
                        substitute(ts.path.root);
                        ts.path.nodes[0] = ts.path.root;
                        ts.str = ts.path.toString();
                        ts.path = VPath(ts.str);
                    }
                }
            }
        }
    }

    request = replace;
}




