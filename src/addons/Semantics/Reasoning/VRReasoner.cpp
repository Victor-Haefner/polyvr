#include "VRReasoner.h"
#include "VROntology.h"
#include "VRProperty.h"

#include <iostream>
#include <sstream>
#include <list>
#include "core/utils/toString.h"
#include "core/gui/VRGuiManager.h"
#include "core/gui/VRGuiConsole.h"

using namespace std;
using namespace OSG;

bool VRReasoner::verbGui = true;
bool VRReasoner::verbConsole = true;
string VRReasoner::pre = "  ?!?  ";
string VRReasoner::redBeg  = "\033[0;38;2;255;150;150m";
string VRReasoner::greenBeg = "\033[0;38;2;150;255;150m";
string VRReasoner::blueBeg = "\033[0;38;2;150;150;255m";
string VRReasoner::yellowBeg = "\033[0;38;2;255;255;150m";
string VRReasoner::colEnd = "\033[0m";

VRReasoner::VRReasoner() {
    verbConsole = false;
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

void VRReasoner::print(const string& s) {
    if (verbConsole) cout << pre << s << endl;
    if (verbGui) VRGuiManager::get()->getConsole( "Reasoning" )->write( s+"\n" );
}

void VRReasoner::print(const string& s, COLOR c) {
    if (verbConsole) {
        switch(c) {
            case BLUE: cout << blueBeg; break;
            case RED: cout << redBeg; break;
            case GREEN: cout << greenBeg; break;
            case YELLOW: cout << yellowBeg; break;
        }
        cout << pre << s << colEnd << endl;
    }

    if (verbGui) {
        switch(c) {
            case BLUE: VRGuiManager::get()->getConsole( "Reasoning" )->write( s+"\n", "blue" );
            case RED: VRGuiManager::get()->getConsole( "Reasoning" )->write( s+"\n", "red" );
            case GREEN: VRGuiManager::get()->getConsole( "Reasoning" )->write( s+"\n", "green" );
            case YELLOW: VRGuiManager::get()->getConsole( "Reasoning" )->write( s+"\n", "yellow" );
        }
    }
}

bool VRReasoner::findRule(VRStatementPtr statement, VRSemanticContextPtr context) {
    print("     search rule for statement: " + statement->toString());
    for ( auto r : context->onto->getRules()) { // no match found -> check rules and initiate new queries
        //print("      check rule in context: "+r->rule, BLUE);
        if (!context->rules.count(r->rule)) continue;
        Query query = context->rules[r->rule];
        //print("      check rule verb: "+query.request->verb+" and "+statement->verb, BLUE);
        if (query.request->verb != statement->verb) continue; // rule verb does not match
        print("      found rule with matching name: " + query.request->toString(), GREEN);
        if (!statement->match(query.request)) continue; // statements are not similar enough
        print("      found rule: " + query.request->toString(), GREEN);

        query.substituteRequest(statement);
        //query.request->updateLocalVariables(context.vars, context.onto);
        context->queries.push_back(query);
        print("      add query " + query.toString(), YELLOW);
        return true;
    }
    print("      no rule found!", RED);
    return false;
}

bool VRReasoner::builtin(VRStatementPtr s, VRSemanticContextPtr c) {
    if (s->terms.size() < 2) return false;
    if (!s->terms[1].var) return false;
    string cb_name = s->terms[0].str;
    if (!c->onto->builtins.count(cb_name)) return false;
    auto& builtin = c->onto->builtins[cb_name];

    vector< map<VREntity*, string> > params; // get parameters

    for (int i=0; i<s->terms.size()-2; i++) {
        auto& t = s->terms[2+i];
        params.push_back( map<VREntity*, string>() );
        //cout << "builtin params in: " << t.str << endl;

        if (t.isMathExpression()) { params[i][0] = t.computeExpression(c); continue; }

        auto r = t.path.root;
        if (c->vars.count(r)) {
            auto r_var = c->vars[r];

            for (auto erp : r_var->entities) {
                auto er = erp.second;
                if (!er) continue;

                auto vv = t.path.getValue(er);
                for (auto v : vv) {
                    if (!c->vars.count(v)) continue;
                    auto e_var = c->vars[v];
                    for (auto ep : e_var->entities) {
                        auto e = ep.second;
                        if (e->is_a("Vector")) params[i][er.get()] = e->get("x")->value +" "+ e->get("y")->value +" "+ e->get("z")->value;
                        else params[i][er.get()] = e->getName();
                    }
                }
            }
            continue;
        }

        if (t.var) {
            for (auto ep : t.var->entities) {
                auto e = ep.second;
                if (!e) continue;
                if (e->is_a("Vector")) params[i][e.get()] = e->get("x")->value +" "+ e->get("y")->value +" "+ e->get("z")->value;
                else params[i][e.get()] = e->getName();
            }
            continue;
        }
    }

    auto& entities = s->terms[1].var->entities;
    for (auto entity : entities) { // apply to entities
        VRObjectPtr obj = entity.second->getSGObject();
        if (!obj) continue;

        vector<string> args;
        for (auto p : params) {
            auto e = entity.second.get();
            //cout << " e in p: " << p.count(e) << " ps: " << p.size() << endl;
            if (p.count(e)) args.push_back(p[e]);
            if (p.count(0)) args.push_back(p[0]);
        }

        //cout << "builtin sg object: " << obj << endl;
        //for (auto a : args) cout << "builtin args: " << a << endl;
        builtin->execute(obj, args);
    }

    return true;
}

bool VRReasoner::is(VRStatementPtr statement, VRSemanticContextPtr context) {
    auto& left = statement->terms[0];
    auto& right = statement->terms[1];
    if ( context->vars.count(left.var->value) == 0) return false; // check if context has a variable with the left value
    if (!left.valid() || !right.valid()) return false; // return if one of the sides invalid

    bool b = left.is(right, context);
    bool NOT = statement->verb_suffix == "not";
    print("   " + left.str + " is " + (b?"":" not ") + (NOT?" not ":"") + right.var->value);

    return ( (b && !NOT) || (!b && NOT) );
}

bool VRReasoner::has(VRStatementPtr statement, VRSemanticContextPtr context) { // TODO
    auto& left = statement->terms[0];
    auto& right = statement->terms[1];

    bool b = left.has(right, context);
    print("  " + left.str + " has " + (b?"":"not") + " " + right.str);
    print("RES " + toString(&right.var) + "   " + right.var->toString());
    if (b) { statement->state = 1; return true; }

    // DEBUG -> TODO

    auto Pconcept = context->onto->getConcept( left.var->concept ); // parent concept
    auto Cconcept = context->onto->getConcept( right.var->concept ); // child concept
    if (Pconcept == 0) { cout << "Warning (has): first concept " << left.var->concept << " not found!\n"; return false; }
    if (Cconcept == 0) { cout << "Warning (has): second concept " << right.var->concept << " not found!\n"; return false; }
    auto prop = Pconcept->getProperties( Cconcept->getName() );
    if (prop.size() == 0) cout << "Warning: has evaluation failed, property " << right.var->value << " missing!\n"; return false;
    return false;
}

// apply the statement changes to world
bool VRReasoner::apply(VRStatementPtr statement, VRSemanticContextPtr context) {
    auto clearAssumptions = [&]() {
        vector<string> toDelete;
        for (auto v : context->vars) {
            if (v.second->isAssumption) toDelete.push_back(v.first);
        }
        for (auto v : toDelete) {
            context->onto->remEntity( context->onto->getEntity(v) );
            //context.vars.erase(v);
        }
    };

    if (statement->verb == "is") {
        auto& left = statement->terms[0];
        auto& right = statement->terms[1];

        if (left.path.size() > 1) {
            for (auto eL : left.var->entities) {
                for (auto eR : right.var->entities) {
                    string vR = "";
                    if (right.isMathExpression()) {
                        vR = right.computeExpression(context);
                    } else {
                        vector<string> valR = right.path.getValue( eR.second );
                        if (valR.size() > 0) vR = valR[0];
                    }

                    left.path.setValue(vR, eL.second);
                    print("  set " + left.str + " to " + right.str + " -> " + vR, GREEN);
                }
            }
        } else {
            left.var->value = right.var->value;
            print("  set " + left.str + " to " + right.str + " -> " + toString(left.var->value), GREEN);
        }
        statement->state = 1;
    }

    if (statement->verb == "has") {
        auto& left = statement->terms[0];
        auto& right = statement->terms[1];
        auto Cconcept = context->onto->getConcept( right.var->concept ); // child concept
        auto Pconcept = context->onto->getConcept( left.var->concept ); // parent concept
        if (!Pconcept || !Cconcept) { print("Warning: failed to apply " + statement->toString()); return false; }
        auto prop = Pconcept->getProperties( Cconcept->getName() );
        if (prop.size() == 0) { print("Warning: failed to apply " + statement->toString()); return false; }
        for (auto i : left.var->entities) i.second->add(prop[0]->getName(), right.var->value); // TODO: the first parameter is wrong
        statement->state = 1;
        print("  give " + right.str + " to " + left.str, GREEN);
    }

    if (statement->verb == "q") {
        if (statement->terms.size() == 0) { print("Warning: failed to apply " + statement->toString() + ", empty query!"); return false; }
        string x = statement->terms[0].var->value;
        VariablePtr v = context->vars[x];
        for (auto e : v->entities) {
            auto& eval = v->evaluations[e.first];
            if (eval.state == Evaluation::VALID) context->results.push_back(e.second);
        }
        statement->state = 1;
        print("  process results of queried variable " + x, GREEN);
        clearAssumptions();
    }

    return true;
}

bool VRReasoner::evaluate(VRStatementPtr statement, VRSemanticContextPtr context) {
    print(" " + toString(statement->place) + " eval " + statement->toString());
    statement->updateLocalVariables(context->vars, context->onto);

    if (statement->verb == "builtin") {
        return builtin(statement, context);
    }

    if (statement->isSimpleVerb()) { // resolve basic verb
        if (statement->verb == "is") return is(statement, context);
        if (statement->verb == "has") return has(statement, context);
    }

    if (statement->terms.size() == 1) { // resolve (anonymous?) variables
        string concept = statement->verb;

        if (auto c = context->onto->getConcept(concept)) {
            string name = statement->terms[0].path.root;
            if (context->vars.count(name)) { // there is already a variable with that name!
                auto var = context->vars[name];
                if ( c->is_a(var->concept) && var->concept != concept ) { // the variable is not the same type or a subtype of concept!
                    //TODO: what happens then?
                    //  are the entities that don't have the concept removed from the variable? I think not...
                }
                print("  reuse variable " + context->vars[name]->toString(), BLUE);
                statement->state = 1;
                return true;
            }

            auto var = Variable::create( context->onto, concept, name );
            context->vars[name] = var;
            print("  added variable " + var->toString(), BLUE);
            statement->state = 1;
            return true;
        }
    }

    return false;
}

vector<VREntityPtr> VRReasoner::process(string initial_query, VROntologyPtr onto) {
    print(initial_query);

    auto context = VRSemanticContext::create(onto); // create context
    context->queries.push_back(Query(initial_query));

    while( context->queries.size() ) { // while queries to process
        Query& query = context->queries.back();
        query.checkState();
        auto request = query.request;
        if (request->state == 1) {
            apply(request, context);
            print(" solved: " + query.toString(), RED);
            context->queries.pop_back(); continue;
        }; // query answered, pop and continue

        print("QUERY " + query.toString(), RED);

        request->updateLocalVariables(context->vars, context->onto);

        for (auto& statement : query.statements) {
            if (statement->state == 1) continue;
            if (evaluate(statement, context)) { statement->state = 1; continue; }
            if (findRule(statement, context)) continue;
            apply(statement, context);
        }

        context->itr++;
        if (context->itr >= context->itr_max) break;
    }

    print(" break after " + toString(context->itr) + " iterations\n");
    for (auto e : context->results) print(" instance " + e->toString());
    return context->results;
}

VRReasonerPtr VRReasoner::create() { return VRReasonerPtr( new VRReasoner() ); }

void VRReasoner::setVerbose(bool gui, bool console) { verbGui = gui; verbConsole = console; }





