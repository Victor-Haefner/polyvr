#include "VRReasoner.h"
#include "VROntology.h"
#include "VRProperty.h"

#include <iostream>
#include <sstream>
#include <list>
#include "core/utils/toString.h"
#include "core/utils/VRCallbackWrapper.h"
#ifndef WITHOUT_GTK
#include "core/gui/VRGuiManager.h"
#include "core/gui/VRGuiConsole.h"
#endif

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

void VRReasoner::setOption(string option, bool b) { options[option] = b; }

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
#ifndef WITHOUT_GTK
    if (verbGui) VRGuiManager::get()->getConsole( "Reasoning" )->write( s+"\n" );
#endif
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

#ifndef WITHOUT_GTK
    if (verbGui) {
        switch(c) {
            case BLUE: VRGuiManager::get()->getConsole( "Reasoning" )->write( s+"\n", "blue" ); break;
            case RED: VRGuiManager::get()->getConsole( "Reasoning" )->write( s+"\n", "red" ); break;
            case GREEN: VRGuiManager::get()->getConsole( "Reasoning" )->write( s+"\n", "green" ); break;
            case YELLOW: VRGuiManager::get()->getConsole( "Reasoning" )->write( s+"\n", "yellow" ); break;
        }
    }
#endif
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
        print("      found rule: " + query.toString(), GREEN);

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

    vector< map<VREntity*, vector<string> > > params; // get parameters

    for (unsigned int i=0; i<s->terms.size()-2; i++) {
        auto& t = s->terms[2+i];
        params.push_back( map<VREntity*, vector<string> >() );
        //cout << "builtin params in: " << t.str << endl;

        if (t.isMathExpression()) { params[i][0] = t.computeMathExpression(c); continue; }

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
                        if (e->is_a("Vector")) params[i][er.get()].push_back( e->asVectorString() );
                        else params[i][er.get()].push_back(e->getName());
                    }
                }
            }
            continue;
        }

        if (t.var) {
            for (auto ep : t.var->entities) {
                auto e = ep.second;
                if (!e) continue;
                if (e->is_a("Vector")) params[i][e.get()].push_back( e->asVectorString() );
                else params[i][e.get()].push_back( e->getName() );
            }
            continue;
        }
    }

    auto& entities = s->terms[1].var->entities;
    for (auto entity : entities) { // apply to entities
        VRObjectPtr obj = entity.second->getSGObject();
        if (!obj) continue;

        vector<string> args;
        for (auto pm : params) {
            auto e = entity.second.get();
            //cout << " e in p: " << p.count(e) << " ps: " << p.size() << endl;
            if (pm.count(e)) for (auto p : pm[e]) args.push_back(p);
            if (pm.count(0)) for (auto p : pm[0]) args.push_back(p);
        }

        //cout << "builtin sg object: " << obj << endl;
        //for (auto a : args) cout << "builtin args: " << a << endl;
        string res;
        builtin->execute(obj.get(), args, res);
    }

    return true;
}

bool VRReasoner::is(VRStatementPtr statement, VRSemanticContextPtr context) {
    auto& left = statement->terms[0];
    auto& right = statement->terms[1];
    for (auto v : left.var->value) if ( context->vars.count(v) == 0) return false; // check if context has a variable with the left value
    if (!left.valid() || !right.valid()) return false; // return if one of the sides invalid

    bool b = left.is(right, context);
    bool NOT = statement->verb_suffix == "not";
    print("   " + left.str + " is" + (b?" ":" not ") + (NOT?" not ":" ") + "'" + right.var->valToString() + "'");

    return ( (b && !NOT) || (!b && NOT) );
}

bool VRReasoner::set(VRStatementPtr statement, VRSemanticContextPtr context) {
    auto& left = statement->terms[0];
    auto& right = statement->terms[1];
    for (auto v : left.var->value) if ( context->vars.count(v) == 0) return false; // check if context has a variable with the left value
    if (!left.valid() || !right.valid()) return false; // return if one of the sides invalid
    return true; // further processed on VRReasoner::apply
}

bool VRReasoner::has(VRStatementPtr statement, VRSemanticContextPtr context) { // TODO
    if (statement->terms.size() < 2) return false;
    auto& left = statement->terms[0];
    auto& right = statement->terms[1];
    if (!right.var || !left.var) return false;

    bool b = left.has(right, context);
    print("  " + left.str + " has " + (b?"":"not") + " " + right.str);
    print("RES " + toString((void*)&right.var) + "   " + right.var->toString());
    if (b) { statement->state = 1; return true; }

    // DEBUG -> TODO

    auto Pconcept = context->onto->getConcept( left.var->concept ); // parent concept
    auto Cconcept = context->onto->getConcept( right.var->concept ); // child concept
    if (Pconcept == 0) { cout << "Warning (has): first concept " << left.var->concept << " not found!\n"; return false; }
    if (Cconcept == 0) { cout << "Warning (has): second concept " << right.var->concept << " not found!\n"; return false; }
    auto prop = Pconcept->getProperties( Cconcept->getName() );
    if (prop.size() == 0) { cout << "Warning: has evaluation failed, property " << right.var->valToString() << " missing!\n"; return false; }
    return false;
}

// apply the statement changes to world
bool VRReasoner::apply(VRStatementPtr statement, Query query, VRSemanticContextPtr context) {
    print("Apply statement " + ::toString((void*)statement.get()) + "  " + statement->toString(), GREEN);

    /*auto clearAssumptions = [&]() {
        vector<string> toDelete;
        for (auto v : context->vars) {
            for (auto e : v.second->entities) {
                if (!v.second->evaluations.count(e.first)) continue;
                auto& eval = v.second->evaluations[e.first];
                if (eval.state == Evaluation::ASSUMPTION) v.second->discard(e.second);
            }
        }
        for (auto v : toDelete) {
            context->onto->remEntity( context->onto->getEntity(v) );
            //context.vars.erase(v);
        }
    };*/

    auto aggr = [](vector<string> v) {
        string r;
        for (unsigned int i=0; i<v.size(); i++) {
            if (i > 0) r += ", ";
            r += v[i];
        }
        return r;
    };

    auto getVariable = [&](string name) -> VariablePtr {
        if (!context->vars.count(name)) {
            print("   Warning: variable " + name + " not known!", RED);
            return 0;
        }
        VariablePtr v = context->vars[name];
        if (!v) {
            print("   Warning: variable " + name + " known but invalid!", RED);
            return 0;
        }
        return v;
    };

    if (statement->verb == "is") {
        if (statement->terms.size() < 2) return false;
        auto& left = statement->terms[0];
        auto& right = statement->terms[1];

        bool lim = left.isMathExpression();
        bool rim = right.isMathExpression();
        vector<string> lmv, rmv;
        if (lim) lmv = left.computeMathExpression(context);
        if (rim) rmv = right.computeMathExpression(context);
        if (lim) print("  left term " + left.str + " is math expression! -> (" + aggr(lmv)+")", GREEN);
        if (rim) print("  right term " + right.str + " is math expression! -> (" + aggr(rmv)+")", GREEN);

        if (left.var && right.var) {
            if (left.path.size() > 1) {
                for (auto eL : left.var->entities) {
                    for (auto eR : right.var->entities) {
                        vector<string> vR;
                        if (rim) vR = rmv;
                        else vR = right.path.getValue( eR.second );

                        if (vR.size()) {
                            left.path.setValue(vR[0], eL.second);
                            print("  set " + left.str + " to " + right.str + " -> " + vR[0], GREEN);
                        }
                    }
                }
            } else {
                left.var->value = (rim && rmv.size()) ? rmv : vector<string>( { right.var->value } );
                print("  set " + left.str + " to " + right.str + " -> " + toString(left.var->value), GREEN);
            }
        }
        //statement->state = 1;
    }

    if (statement->verb == "has" && statement->terms.size() >= 2) {
        auto& left = statement->terms[0];
        auto& right = statement->terms[1];
        auto Cconcept = context->onto->getConcept( right.var->concept ); // child concept
        auto Pconcept = context->onto->getConcept( left.var->concept ); // parent concept
        if (!Pconcept || !Cconcept) { print("Warning: failed to apply " + statement->toString()); return false; }
        auto prop = Pconcept->getProperties( Cconcept->getName() );
        if (prop.size() == 0) { print("Warning: failed to apply " + statement->toString()); return false; }
        if (right.var->value.size()) for (auto i : left.var->entities) i.second->add(prop[0]->getName(), right.var->value[0]); // TODO: the first parameter is wrong
        statement->state = 1;
        print("  give " + right.str + " to " + left.str, GREEN);
    }

    if (statement->verb == "set" && statement->terms.size() >= 2) {
        auto& left = statement->terms[0];
        auto& right = statement->terms[1];
        print(" try apply set to " + left.str);

        bool lim = left.isMathExpression();
        bool rim = right.isMathExpression();
        vector<string> lmv, rmv;
        if (lim) lmv = left.computeMathExpression(context);
        if (rim) rmv = right.computeMathExpression(context);
        if (lim) print("  left term " + left.str + " is math expression! -> (" + aggr(lmv)+")", GREEN);
        if (rim) print("  right term " + right.str + " is math expression! -> (" + aggr(rmv)+")", GREEN);

        auto applySet = [&](VREntityPtr eL, VREntityPtr eR) {
            vector<string> vR;
            if (rim) vR = rmv;
            else vR = right.path.getValue( eR );
            if (vR.size()) {
                left.path.setValue(vR[0], eL);
                print("  set entity " + eL->toString(), GREEN);
                print("  set " + left.str + " to " + right.str + " -> " + vR[0], GREEN);
            }
        };

        if (left.var && right.var) {
            if (left.path.size() > 1) {
                auto ents1 = left.var->getEntities(Evaluation::VALID);
                auto ents2 = right.var->getEntities(Evaluation::VALID);

                if (ents1.size() == ents2.size()) {
                    for (unsigned int i=0; i<ents1.size(); i++) applySet(ents1[i], ents2[i]);
                } else {
                    for (auto eL : ents1) {
                        if (ents2.size() > 0) for (auto eR : ents2) applySet(eL, eR);
                        else {
                            left.path.setValue(right.var->value[0], eL);
                            print("  set " + left.str + " to " + right.str, GREEN);
                        }
                    }
                }

            } else {
                left.var->value = (rim && rmv.size()) ? rmv : vector<string>( { right.var->value } );
                print("  set " + left.str + " to " + right.str + " -> " + toString(left.var->value), GREEN);
            }
        }
        statement->state = 1; // at least wait to find someone!
    }

    if (statement->constructor && statement->terms.size() >= 1) { // 'Error(e) : Event(v) ; is(v.name,crash)'
        string concept = statement->verb;
        string x = statement->terms[0].var->value[0];
        VariablePtr v = getVariable(x);
        if (!v) return false;

        //auto query = statement->constructor->query;
        print("  apply construction rule " + query.toString(), GREEN);
        auto statements = query.statements;
        for (auto s : statements) {
            if (s->terms.size() > 1) continue; // only variable declarations
            auto v2 = getVariable( s->terms[0].var->value[0] );
            if (!context->onto->getConcept(s->verb)) continue; // check for concept
            print("   construction variable found: " + v2->toString(), GREEN);
            for (auto E : v2->getEntities(Evaluation::VALID)) {
                auto e = context->onto->addEntity(x, concept);
                v->addEntity(e);
                print("    construct entity " + e->toString(), GREEN);
            }
        }

        for (auto s : statements) { // apply set
            if (s->terms.size() == 1) continue; // not variable declarations
            apply(s, query, context);
        }
    }

    if (statement->verb == "q") {
        if (statement->terms.size() == 0) { print("Warning: failed to apply " + statement->toString() + ", empty query!"); return false; }
        string x = statement->terms[0].var->value[0];
        print("  process results of queried variable " + x, GREEN);
        VariablePtr v = getVariable(x);
        if (!v) return false;

        bool addAssumtions = true;
        for (auto e : v->evaluations) if(e.second.state == Evaluation::VALID) addAssumtions = false;
        print("   add assumptions? " + toString(addAssumtions), BLUE);
        if (addAssumtions) v->addAssumption(context, x); // TODO
        print("   now processing variable: " + v->toString(), BLUE);
        //cout << "query variable: " << v->toString() << endl;

        for (auto e : v->entities) {
            if (!v->evaluations.count(e.first)) {
                print("    entity " + e.second->toString() + " has no evaluation!", RED);
                continue; // ewntity has no evaluation
            }
            auto& eval = v->evaluations[e.first];
            bool valid = (eval.state == Evaluation::VALID || (addAssumtions && eval.state != Evaluation::INVALID));
            print("    entity " + e.second->toString() + " evaluation: " + ::toString(valid), BLUE);
            if (valid) {
                print("    add valid entity: " + e.second->toString(), GREEN);
                context->results.push_back(e.second);
            }
        }
        statement->state = 1;
        //clearAssumptions(); //TODO
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
        if (statement->verb == "set") return set(statement, context);
    }

    if (statement->terms.size() == 1) { // resolve (anonymous?) variables
        string concept = statement->verb;

        auto findConstructorRules = [&](VRStatementPtr statement, VRSemanticContextPtr context) {
            print("     search constructors for statement: " + statement->toString());
            for ( auto r : context->onto->getRules()) { // no match found -> check rules and initiate new queries
                if (!context->rules.count(r->rule)) continue;
                Query query = context->rules[r->rule];
                if (query.request->verb != statement->verb) continue; // rule verb does not match
                if (!statement->match(query.request)) continue; // statements are not similar enough

                query.substituteRequest(statement);
                context->queries.push_back(query);
                if (!statement->constructor) statement->constructor = ConstructorPtr(new Constructor());
                statement->constructor->query = query;
                print("      found constructor: " + query.toString(), BLUE);
            }
        };

        if (auto c = context->onto->getConcept(concept)) {
            findConstructorRules(statement, context);

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

            auto var = Variable::create( context->onto, concept, {name}, context );
            context->vars[name] = var;
            print("  added variable " + var->toString(), BLUE);
            statement->state = 1;
            return true;
        }
    }

    return false;
}

bool VRReasoner::processQuery(Query& query, VRSemanticContextPtr context) {
    query.checkState();
    auto request = query.request;
    if (!request) {
        print(" ERROR, request is null: " + query.toString(), RED);
        return false;
    }

    if (request->state == 1) {
        apply(request, query, context);
        print(" solved: " + query.toString(), RED);
        context->queries.pop_back(); return false;
    }; // query answered, pop and continue

    print("QUERY " + query.toString(), RED);

    request->updateLocalVariables(context->vars, context->onto);

    for (auto& statement : query.statements) {
        if (statement->state == 1) continue;
        if (evaluate(statement, context)) { statement->state = 1; print("   evaluated successfully!"); continue; }
        if (findRule(statement, context)) continue;
        apply(statement, query, context);
    }
    return true;
}

vector<VREntityPtr> VRReasoner::process(string strQuery, VROntologyPtr onto) {
    print(strQuery);

    auto context = VRSemanticContext::create(onto); // create context
    context->options = options;
    context->queries.push_back(Query(strQuery));
    Query initial_query = context->queries.back();

    print("reasoning context options:");
    for (auto opt : options) {
        print(" " + opt.first + " : " + toString(opt.second));
    }

    vector<Query*> queryHash;

    auto checkStale = [&] {
        vector<Query*> newHash;
        for (auto& q : context->queries) newHash.push_back(&q);
        auto tmpHash = queryHash;
        queryHash = newHash;

        if (newHash.size() != tmpHash.size()) return false;
        for (size_t i=0; i<newHash.size(); i++) if (newHash[i] != tmpHash[i]) return false;
        return true;
    };

    while( context->queries.size() ) { // while queries to process
        print("\n");
        Query& query = context->queries.back();
        if (!processQuery(query, context)) continue;

        bool stale = checkStale();
        print("stale? " + toString(stale));
        if (stale) context->itr_stale++;
        if (context->itr_stale >= context->itr_max_stale) break;

        context->itr++;
        if (context->itr >= context->itr_max) break;
    }

    print("\nFinish after " + toString(context->itr) + " iterations\n");
    for (auto e : context->results) print(" instance " + e->toString());

    if (initial_query.request->state != 1) context->results.clear(); // invalidate results if the query was not fully answered
    return context->results;
}

VRReasonerPtr VRReasoner::create() { return VRReasonerPtr( new VRReasoner() ); }

void VRReasoner::setVerbose(bool gui, bool console) { verbGui = gui; verbConsole = console; }





