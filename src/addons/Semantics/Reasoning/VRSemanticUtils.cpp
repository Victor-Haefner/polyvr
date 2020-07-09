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
#include <OpenSG/OSGVector.h>

using namespace OSG;

string Evaluation::toString() {
    if (state == ASSUMPTION) return "assumption";
    if (state == INVALID) return "invalid";
    if (state == VALID) return "valid";
    return "invalid";
}

Variable::Variable() {;}

string Variable::toString() {
    string s = valToString()+" (" + concept + "){";
    for (auto i : entities) s += i.second->getName()+",";
    for (auto i : evaluations) s += i.second.toString()+",";
    if (entities.size() > 0) s.pop_back();
    s +="}[";
    if (isAnonymous) s += "anonymous, ";
    s += valid ? "valid" : "invalid";
    s +="]";
    return s;
}

string Variable::valToString() {
    string r;
    for (unsigned int i=0; i<value.size(); i++) {
        if (i > 0) r += ", ";
        r += value[i];
    }
    return r;
}

/* Variable flags:
    isAnonymous: no entities with that name have been found
    !valid: something is terribly wrong!
*/

Variable::Variable(VROntologyPtr onto, string concept, vector<string> var, VRSemanticContextPtr context) {
    auto cl = onto->getConcept(concept);
    if (cl == 0) return;

    this->concept = concept; // TODO: maybe the entity has a concept that inherits from the concept passed above?
    value = var;

    for (auto v : value) {
        if ( auto i = onto->getEntity(v) ) {
            addEntity(i);
            isAnonymous = false;
        } else { // get all entities of the required type
            for (auto i : onto->getEntities(concept)) addEntity(i);
            if (entities.size() == 0) addAssumption(context, v);
        }
    }

    valid = true;
}

Variable::Variable(VROntologyPtr onto, vector<string> val) {
    value = val;
}

shared_ptr<Variable> Variable::create(VROntologyPtr onto, string concept, vector<string> var, VRSemanticContextPtr context) { return shared_ptr<Variable>( new Variable(onto, concept, var, context) ); }
shared_ptr<Variable> Variable::create(VROntologyPtr onto, vector<string> val) { return shared_ptr<Variable>( new Variable(onto, val) ); }

void Variable::addAssumption(VRSemanticContextPtr context, string var) {
    if (context->getOption("allowAssumptions")) {
        auto c = context->onto->getConcept(concept);
        auto e = context->onto->addEntity(var, concept);
        addEntity(e, true);
        if (c->is_a("Vector")) {
            Vec3d v;
            bool b = toValue(value[0], v);
            if (b) {
                e->set("x", ::toString(v[0]));
                e->set("y", ::toString(v[1]));
                e->set("z", ::toString(v[2]));
            }
        }

        if (c->is_a("float")) {
            float f;
            bool b = toValue(value[0], f);
            if (b) e->set("var", ::toString(f));
        }

        if (c->is_a("int")) {
            int f;
            bool b = toValue(value[0], f);
            if (b) e->set("var", ::toString(f));
        }

        if (c->is_a("string")) {
            e->set("var", value[0]);
        }
    }
}

void Variable::addEntity(VREntityPtr e, bool assumtion) {
    entities[e->ID] = e;
    evaluations[e->ID] = Evaluation();
    if (assumtion) evaluations[e->ID].state = Evaluation::ASSUMPTION;
}

bool Variable::has(VariablePtr other, VPath& path1, VPath& path2, VROntologyPtr onto) {
    if (!other) return false;

    map<VREntityPtr, vector<VREntityPtr>> matches;
    map<VREntity*, bool> visited;

    function<bool(VREntityPtr, string&, string)> computeMatches = [&](VREntityPtr e, string& oName, string indent) -> bool {
        if (!e) return false;
        if (visited.count(e.get())) return false; // check for cycles / visited graph nodes
        visited[e.get()] = true;
        VRReasoner::print( indent+"       search match with " + oName + " for entity: " + e->toString());

        for (auto p : e->properties) { // property vectors of local entity
            for (auto v : p.second) { // local properties
                //cout << " prop: " << v->value << " " << oName << " " << bool(v->value == oName) << endl;
                //if (v->value == other->value) matches[e].push_back(0); // TODO: direct match with other variable value
                if (v.second->value == oName) {
                    VRReasoner::print( indent+"        found match! " + v.second->value + " and " + oName);
                    return true;
                }

                // recursive has! has advantages and disadvantages!!
                // TODO: make it optional in semantics context!
                // TODO: maybe introduce a cycles detection?
                auto childEntity = onto->getEntity(v.second->value);
                if (evaluations.count(e->ID))
                    if (evaluations[e->ID].state == Evaluation::INVALID) continue;
                if (childEntity) VRReasoner::print( indent+"        computeMatches recursion, val: " + v.second->value + " and entity: " + childEntity->getName());
                if (computeMatches(childEntity, oName, indent+"  ")) return true;
            }
        }
        return false;
    };

    // get all matches
    VRReasoner::print( "    Variable::has, variable1: " + toString() + " at path " + path1.toString());
    VRReasoner::print( "    Variable::has, variable2: " + other->toString() + " at path " + path2.toString());
    for (auto i2 : other->entities) { // check each instance of the other variable
        if (other->evaluations[i2.first].state == Evaluation::INVALID) continue;
        string oName = i2.second->getName();
        VRReasoner::print( "     check for match ent2: " + i2.second->toString());
        for (auto i1 : entities) { // all entities of that variable
            if (evaluations[i1.first].state == Evaluation::INVALID) continue;
            VRReasoner::print( "      check for match ent1: " + i1.second->toString());
            for (auto v : path1.getValue(i1.second)) {
                auto e = onto->getEntity(v);
                VRReasoner::print( "       check for match ent1 at path1: " + e->toString());
                if (evaluations.count(e->ID))
                    if (evaluations[e->ID].state == Evaluation::INVALID) continue;
                bool doMatch = computeMatches(e, oName, "");
                VRReasoner::print( "       check for match val1: " + v);
                VRReasoner::print( "       var1 has " + oName + " ? -> " + ::toString(doMatch));
                if (doMatch) matches[i1.second].push_back(i2.second);
            }
        }
        visited.clear();
    }

    // remove non matched entities
    for (auto e : entities) { // all entities of that variable
        if (evaluations[e.first].state == Evaluation::INVALID) continue;
        if (matches.count(e.second) == 0) {
            evaluations[e.first].state = Evaluation::INVALID;
            VRReasoner::print( "     invalidate1 entity " + e.second->getName() );
        }
    }

    for (auto i2 : other->entities) { // check each entity of the other variable
        if (other->evaluations[i2.first].state == Evaluation::INVALID) continue;
        bool found = false;
        for (auto ev : matches) {
            for (auto e : ev.second) {
                found = (e == i2.second);
                if (found) break;
            }
            if (found) break;
        }
        if (!found) {
            other->evaluations[i2.first].state = Evaluation::INVALID;
            VRReasoner::print( "     invalidate2 entity " + i2.second->getName() );
        }
    }

    VRReasoner::print( "     Variable::has, variable1: " + toString() + " at path " + path1.toString());
    VRReasoner::print( "     Variable::has, variable2: " + other->toString() + " at path " + path2.toString());

    return (matches.size() > 0);
}

bool Variable::is(VariablePtr other, VPath& path1, VPath& path2) {
    if (!valid || !other->valid) return false;
    //VRReasoner::print( " Variable::is? " + toString() + "   /   " + other->toString() );

    auto hasSameVal = [&](vector<string>& val1, vector<string>& val2) {
        VRReasoner::print( "       hasSameVal? " + ::toString(Vec2i(val1.size(), val2.size())) );
        for (string s1 : val1) {
            VRReasoner::print( "        str1 " + s1 );
            for (string s2 : val2) {
                VRReasoner::print( "         str2 " + s2 );
                if (s1 == s2) return true;
            }
        }
        VRReasoner::print( "       nope");
        return false;
    };

    auto hasSameVal2 = [&](vector<string>& val1) {
        bool res = false;
        for (auto e : other->entities) {
            if (other->evaluations[e.first].state == Evaluation::INVALID) continue;
            VRReasoner::print( "      other entity: " + e.second->toString() );
            vector<string> val2 = path2.getValue(e.second);
            for (auto v : val2) VRReasoner::print( "       var2 value: " + v );
            auto r = hasSameVal(val1, val2);
            if (!r) {
                other->evaluations[e.first].state = Evaluation::INVALID;
                VRReasoner::print( "     invalidate2 entity " + e.second->getName() );
            }
            if (r) res = true;
        }
        if (res) return true;

        for (string s : val1) {
            VRReasoner::print( "      val1: '" + s + "'");
            for (string v : other->value) {
                VRReasoner::print( "      val2: '" + v + "' -> " + ::toString(bool(s == v)) );
                if (s == v) return true;
            }
        }
        return false;
    };

    VRReasoner::print( "    Variable::is, variable1: " + toString() + " at path " + path1.toString());
    VRReasoner::print( "    Variable::is, variable2: " + other->toString() + " at path " + path2.toString());
    bool res = false;
    for (auto e : entities) {
        if (evaluations[e.first].state == Evaluation::INVALID) continue;
        VRReasoner::print( "     check entity: " + e.second->getName() );
        vector<string> val1 = path1.getValue(e.second);
        for (auto v : val1) VRReasoner::print( "      value at path1 (val1) : " + v );
        auto r = hasSameVal2(val1);
        if (!r) {
            VRReasoner::print( "     invalidate1 entity " + e.second->getName() );
            evaluations[e.first].state = Evaluation::INVALID;
        }
        if (r) res = true;
    }
    VRReasoner::print( "     Variable::is, variable1: " + toString() + " at path " + path1.toString());
    VRReasoner::print( "     Variable::is, variable2: " + other->toString() + " at path " + path2.toString());
    return res;
}

bool Variable::operator==(Variable v) {
    if (!v.valid || !valid) return false;
    for (unsigned int i=0; i<entities.size(); i++) {
        if (v.entities[i] != entities[i]) return false;
    }
    return true;
}

void Variable::discard(VREntityPtr e) {
    if (!entities.count(e->ID)) return;
    entities.erase(e->ID);
    evaluations.erase(e->ID);
}

vector<VREntityPtr> Variable::getEntities(Evaluation::STATE state) {
    vector<VREntityPtr> res;
    for (auto e : entities) {
        if (!evaluations.count(e.first)) {
            VRReasoner::print("    entity " + e.second->toString() + " has no evaluation!", VRReasoner::RED);
            continue; // ewntity has no evaluation
        }
        auto& eval = evaluations[e.first];
        bool valid = (eval.state == state || state == Evaluation::ALL);
        if (valid) res.push_back(e.second);
    }
    return res;
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

    if (size() == 1) {
        if (auto var = e->get("var")) {
            if (e->is_a("float")) res.push_back( var->value );
            if (e->is_a("int")) res.push_back( var->value );
            if (e->is_a("string")) res.push_back( var->value );
        }
        if (e->is_a("Vector")) res.push_back( e->asVectorString() );
        else res.push_back( e->getName() );
        return res;
    }

    auto getSubSet = [&](string& m, int& k) {
        auto s1 = splitString(m,'[');
        if (s1.size() != 2) return false;
        m = s1[0];
        k = toInt( splitString(s1[1],']')[0] );
        return true;
    };

    auto getEntityProperties = [&](VREntityPtr e, string m) {
        int k = 0;
        bool doSubset = getSubSet(m, k);
        vector<VRPropertyPtr> res;
        auto prop = e->getProperty(m, true);
        if (!prop) return res;
        if (!e->properties.count(prop->getName())) return res;
        for (auto p : e->properties[prop->getName()]) res.push_back(p.second);
        if (!doSubset) return res;
        if (k < 0) k+=res.size();
        return vector<VRPropertyPtr>({ res[k] });
    };

    function<void(VREntityPtr,int,string)> traversePath = [&](VREntityPtr e, int i,string padding) -> void {
        if (i == size()-1) { // finish
            //if (size() > 3) cout << padding << " traversePath finish " << i << " for " << nodes[i] << endl;
            for (auto p : getEntityProperties(e,nodes[i])) {
                //if (size() > 3) cout << padding << "  traversePath found " << p->value << endl;
                res.push_back(p->value);
            }
        } else { // traverse
            //if (size() > 3) cout << padding << " traversePath continue " << i << " for " << nodes[i] << endl;
            for (auto p : getEntityProperties(e,nodes[i])) {
                //if (size() > 3) cout << padding << "  traversePath " << i << " p " << p->value << endl;
                auto e2 = onto->getEntity(p->value);
                if (!e2) continue;
                traversePath(e2,i+1,padding+"  ");
            }
        }
    };

    //if (size() > 3) cout << "VPath::getValue " << toString() << endl;
    traversePath(e,1,"");
    return res;
}

void VPath::setValue(string v, VREntityPtr e) {
    if (size() == 2) {
        string m = first;
        auto prop = e->getProperty(m, true);
        if (!prop) return;
        if (!e->properties.count(prop->getName())) e->set(prop->getName(), "");
        for (auto p : e->properties[prop->getName()]) p.second->setValue( v );
    }
}

VRSemanticContext::VRSemanticContext(VROntologyPtr onto) {
    this->onto = onto;
}

void VRSemanticContext::init() {
    if (!onto) return;

    //cout << "Init VRSemanticContext:" << endl;
    for (auto i : onto->entities) {
        if (i.second->getConcepts().size() == 0) { cout << "VRSemanticContext::VRSemanticContext instance " << i.second->getName() << " has no concepts!" << endl; continue; }
        vars[i.second->getName()] = Variable::create( onto, i.second->getConcepts()[0]->getName(), { i.second->getName() }, ptr() );
        //cout << " add instance " << i.second->toString() << endl;
    }

    for ( auto r : onto->getRules()) {
        //cout << "VRSemanticContext prep rule " << r->rule << endl;
        Query q(r->toString());
        if (!q.request) continue;

        for (Term& t : q.request->terms) {
            t.var = Variable::create(onto,{t.path.root});

            for (auto& s : q.statements) {
                if (s->isSimpleVerb()) continue;
                s->terms[0].var = Variable::create(onto, {s->terms[0].path.root});
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

VRSemanticContextPtr VRSemanticContext::ptr() { return static_pointer_cast<VRSemanticContext>( shared_from_this() ); }

VRSemanticContextPtr VRSemanticContext::create(VROntologyPtr onto) {
    auto context = VRSemanticContextPtr( new VRSemanticContext(onto) );
    context->init();
    return context;
}

// TODO: parse concept statements here
Query::Query() {}

Query::Query(string q) {
    vector<string> parts = VRReasoner::split(q, ':');
    if (parts.size() > 0) request = VRStatement::create(parts[0]);
    if (parts.size() > 1) {
        parts = VRReasoner::split(parts[1], ';');
        for (unsigned int i=0; i<parts.size(); i++) statements.push_back(VRStatement::create(parts[i], i));
    }
}

string Query::toString() {
    string res = request->toString() + " :";
    for (auto s : statements) res += " " + s->toString();
    return res;
}

Term::Term(string s) : path(s), str(s) {}

bool Term::isMathExpression() { MathExpression e(str); return e.isMathExpression(); }

vector<string> Term::computeMathExpression(VRSemanticContextPtr context) {
    MathExpression me(str);
    if (!me.isMathExpression()) return vector<string>();
    me.parse(); // build RDP tree

    cout << "Term::computeMathExpression " << str << endl;
    cout << "Term::computeMathExpression tree:\n" << me.toString() << endl;
    vector<vector<string>> valuesMap;
    for (auto l : me.getLeafs()) {
        VPath p(l->chunk);
        vector<string> values;
        cout << " expression leaf " << l->chunk << endl;
        if (context->vars.count(p.root)) {
            auto v = context->vars[p.root];
            for (auto e : v->entities) {
                cout << "  v entity " << e.second->toString() << endl;
                if (e.second->is_a("Vector")) {
                    auto props = e.second->getAll();
                    vector<string> vec(props.size());
                    for (unsigned int i=0; i<props.size(); i++) vec[i] = props[i]->value;
                    values.push_back( "["+vec[0]+","+vec[1]+","+vec[2]+"]" );
                } else {
                    auto vals = p.getValue(e.second);
                    for (auto val : vals) values.push_back(val);
                }
            }
        }

        if (values.size() == 0) values.push_back(p.root); // default is to use path root, might just be a number
        valuesMap.push_back(values);
        //for (auto val : values) l->setValue(val);
        cout << "  expression leaf final " << l->toString() << ", from " << values.size() << " values!" << endl;
    }

    vector<string> res;
    auto leafs = me.getLeafs();
    int N = leafs.size();
    vector<int> config(N, 0);

    auto setConfig = [&]() { // set a value configuration, compute and push result
        for (unsigned int i=0; i<valuesMap.size(); i++) {
            leafs[i]->setValue( valuesMap[i][config[i]] );
        }
        res.push_back( me.compute() );
    };

    function<void(int)> aggregate = [&](int k) {
        for (unsigned int i=0; i<valuesMap[k].size(); i++) {
            config[k] = i;
            if (k == N-1) setConfig();
            else aggregate(k+1);
        }
    };

    aggregate(0);

    return res;
}

bool Term::valid() { return var ? var->valid : false; }

bool Term::is(Term& t, VRSemanticContextPtr context) {
    auto v = t.var;
    if (t.isMathExpression()) {
        auto res = t.computeMathExpression(context);
        v = Variable::create(0,res);
    }
    return var->is(v, path, t.path);
}

bool Term::has(Term& t, VRSemanticContextPtr context) {
    if (!var) return false;
    auto v = t.var;
    /*if (t.isMathExpression()) { // TODO: sure this does not apply?
        auto res = t.computeMathExpression(context);
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
    for (unsigned int i=0; i<request->terms.size(); i++) {
        Term& t1 = request->terms[i];
        Term& t2 = replace->terms[i];
        substitutes[t1.var->value[0]] = t2.str;
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
        cout << " substitute statement " << statement->toString() << endl;
        for (auto& ts : statement->terms) {
            cout << "  substitute term " << ts.str << endl;
            if (ts.isMathExpression()) {
                MathExpression e(ts.str);
                e.parse();
                cout << "   substitute expression: " << e.toString() << " leafs: " << e.getLeafs().size() << endl;
                for (auto& l : e.getLeafs()) {
                    cout << "    substitute leaf: " << l->chunk << endl;
                    /*for (unsigned int i=0; i<request->terms.size(); i++) {
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
                    }*/
                    if (ts.path.root == l->chunk) {
                        cout << "     substitute 1 param: " << l->chunk << endl;
                        substitute(l->chunk);
                    } else {
                        VPath lpath(l->chunk);
                        cout << "     substitute 2 param: " << lpath.root << endl;
                        substitute(lpath.root);
                        lpath.nodes[0] = lpath.root;
                        l->chunk = lpath.toString();
                    }
                }
                ts.str = e.toString();
                ts.path = VPath(ts.str);
                cout << "    substituted expression: " << ts.str << endl;
            } else {
                /*for (unsigned int i=0; i<request->terms.size(); i++) { // this has trouble when verbs are identical
                    auto& t1 = request->terms[i];
                    if (t1.path.root == ts.path.root) {
                        substitute(ts.path.root);
                        ts.path.nodes[0] = ts.path.root;
                        ts.str = ts.path.toString();
                        ts.path = VPath(ts.str);
                    }
                }*/
                substitute(ts.path.root);
                ts.path.nodes[0] = ts.path.root;
                ts.str = ts.path.toString();
                ts.path = VPath(ts.str);
                cout << "    substituted term: " << ts.str << endl;
            }
        }
    }

    request = replace;
}

bool VRSemanticContext::getOption(string option) {
    if (options.count(option)) return options[option];
    return false;
}



