#include "VRReasoner.h"
#include "VROntology.h"
#include "VRProperty.h"

#include <iostream>
#include <sstream>
#include <list>
#include "core/utils/toString.h"
#include "core/gui/VRGuiManager.h"

using namespace std;
using namespace OSG;

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

/* TODO
 - non matching paths may be valid rule!

*/

void VRReasoner::print(const string& s) {
    if (verbConsole) cout << pre << s << endl;
    if (verbGui) VRGuiManager::get()->printToConsole( "Reasoning", s+"\n" );
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

    if (verbGui) VRGuiManager::get()->printToConsole( "Reasoning", s+"\n" ); // TODO
}

bool VRReasoner::findRule(VRStatementPtr statement, Context& context) {
    print("     search rule for statement: " + statement->toString());
    for ( auto r : context.onto->getRules()) { // no match found -> check rules and initiate new queries
        if (!context.rules.count(r->rule)) continue;
        Query query = context.rules[r->rule];
        if (query.request->verb != statement->verb) continue; // rule verb does not match
        print("      found rule: " + query.request->toString(), GREEN);
        if (!statement->match(query.request)) continue; // statements are not similar enough
        //query.request.updateLocalVariables(context.vars, context.onto);

        query.substituteRequest(statement);
        context.queries.push_back(query);
        print("      add query " + query.toString(), YELLOW);
        return true;
    }
    print("      no rule found!", RED);
    return false;
}

bool VRReasoner::is(VRStatementPtr statement, Context& context) {
    auto& left = statement->terms[0];
    auto& right = statement->terms[1];
    if ( context.vars.count(left.var->value) == 0) return false; // check if context has a variable with the left value
    if (!left.valid() || !right.valid()) return false; // return if one of the sides invalid

    bool b = left.var->is(right.var, left.path, right.path);
    bool NOT = statement->verb_suffix == "not";
    print("   " + left.str + " is " + (b?"":" not ") + (NOT?" not ":"") + right.var->value);

    return ( (b && !NOT) || (!b && NOT) );
}

bool VRReasoner::has(VRStatementPtr statement, Context& context) { // TODO
    auto& left = statement->terms[0];
    auto& right = statement->terms[1];

    bool b = left.var->has(right.var, context.onto);
    print("  " + left.str + " has " + (b?"":"not") + " " + right.str);
    print("RES " + toString(&right.var) + "   " + right.var->toString());
    if (b) { statement->state = 1; return true; }

    // DEBUG -> TODO

    auto Pconcept = context.onto->getConcept( left.var->concept ); // parent concept
    auto Cconcept = context.onto->getConcept( right.var->concept ); // child concept
    if (Pconcept == 0) { cout << "Warning (has): first concept " << left.var->concept << " not found!\n"; return false; }
    if (Cconcept == 0) { cout << "Warning (has): second concept " << right.var->concept << " not found!\n"; return false; }
    auto prop = Pconcept->getProperties( Cconcept->getName() );
    if (prop.size() == 0) cout << "Warning: has evaluation failed, property " << right.var->value << " missing!\n"; return false;
    return false;
}

// apply the statement changes to world
bool VRReasoner::apply(VRStatementPtr statement, Context& context) {
    auto clearAssumptions = [&]() {
        vector<string> toDelete;
        for (auto v : context.vars) {
            if (v.second->isAssumption) toDelete.push_back(v.first);
        }
        for (auto v : toDelete) {
            context.onto->remEntity( context.onto->getEntity(v) );
            //context.vars.erase(v);
        }
    };

    if (statement->verb == "is") {
        auto& left = statement->terms[0];
        auto& right = statement->terms[1];
        left.var->value = right.var->value; // TOCHECK
        statement->state = 1;
        print("  set " + left.str + " to " + right.str, GREEN);
    }

    if (statement->verb == "has") {
        auto& left = statement->terms[0];
        auto& right = statement->terms[1];
        auto Cconcept = context.onto->getConcept( right.var->concept ); // child concept
        auto Pconcept = context.onto->getConcept( left.var->concept ); // parent concept
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
        VariablePtr v = context.vars[x];
        for (auto e : v->entities) {
            auto& eval = v->evaluations[e.first];
            if (eval.state == Evaluation::VALID) context.results.push_back(e.second);
        }
        statement->state = 1;
        print("  process results of queried variable " + x, GREEN);
        clearAssumptions();
    }

    return true;
}

bool VRReasoner::evaluate(VRStatementPtr statement, Context& context) {
    print(" " + toString(statement->place) + " eval " + statement->toString());
    statement->updateLocalVariables(context.vars, context.onto);

    if (statement->isSimpleVerb()) { // resolve (anonymous?) variables
        if (statement->verb == "is") return is(statement, context);
        if (statement->verb == "has") return has(statement, context);
    }

    if (statement->terms.size() == 1) { // resolve (anonymous?) variables
        string concept = statement->verb;
        if (context.onto->getConcept(concept)) {
            string name = statement->terms[0].path.root;
            auto var = Variable::create( context.onto, concept, name );
            context.vars[name] = var;
            print("  added variable " + var->toString(), BLUE);
            statement->state = 1;
            return true;
        }
    }

    return false;
}
    // TODO: introduce requirements rules for the existence of some individuals

vector<VREntityPtr> VRReasoner::process(string initial_query, VROntologyPtr onto) {
    print(initial_query);

    Context context(onto); // create context
    context.queries.push_back(Query(initial_query));

    while( context.queries.size() ) { // while queries to process
        Query& query = context.queries.back();
        query.checkState();
        auto request = query.request;
        if (request->state == 1) {
            apply(request, context);
            print(" solved: " + query.toString(), RED);
            context.queries.pop_back(); continue;
        }; // query answered, pop and continue

        print("QUERY " + query.toString(), RED);

        request->updateLocalVariables(context.vars, context.onto);

        for (auto& statement : query.statements) {
            if (statement->state == 1) continue;
            if (evaluate(statement, context)) { statement->state = 1; continue; }
            if (findRule(statement, context)) continue;
            apply(statement, context);
        }

        context.itr++;
        if (context.itr >= context.itr_max) break;
    }

    print(" break after " + toString(context.itr) + " iterations\n");
    for (auto e : context.results) print(" instance " + e->toString());
    return context.results;
}

VRReasonerPtr VRReasoner::create() { return VRReasonerPtr( new VRReasoner() ); }

void VRReasoner::setVerbose(bool gui, bool console) { verbGui = gui; verbConsole = console; }





