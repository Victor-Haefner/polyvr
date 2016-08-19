#include "VRReasoner.h"
#include "VROntology.h"
#include "VRProperty.h"

#include <iostream>
#include <sstream>
#include <list>
#include "core/utils/toString.h"

using namespace std;
using namespace OSG;

VRReasoner::VRReasoner() {;}

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

bool VRReasoner::findRule(VRStatementPtr statement, Context& context) {
    cout << pre << "     search rule for " << statement->toString() << endl;
    for ( auto r : context.onto->getRules()) { // no match found -> check rules and initiate new queries
        if (!context.rules.count(r->rule)) continue;
        Query query = context.rules[r->rule];
        if (query.request->verb != statement->verb) continue; // rule verb does not match
        cout << pre << "      rule " << query.request->toString() << endl;

        //query.request.updateLocalVariables(context.vars, context.onto);
        if (!statement->match(query.request)) continue; // statements are not similar enough

        query.request = statement; // TODO: use pointer? new query needs to share ownership of the statement
        context.queries.push_back(query);
        cout << pre << yellowBeg << "      add query " << query.toString() << colEnd << endl;
        return true;
    }
    cout << pre << "      no rule found " << endl;
    return false;
}

bool VRReasoner::is(VRStatementPtr statement, Context& context) {
    auto& left = statement->terms[0];
    auto& right = statement->terms[1];
    if ( context.vars.count(left.var->value) == 0) return false; // check if context has a variable with the left value
    if (!left.valid() || !right.valid()) return false; // return if one of the sides invalid

    bool b = left == right;
    bool NOT = statement->verb_suffix == "not";
    cout << pre << "   " << left.str << " is " << (b?"":" not ") << (NOT?" not ":"") << right.var->value << endl;

    return ( (b && !NOT) || (!b && NOT) );
}

bool VRReasoner::has(VRStatementPtr statement, Context& context) { // TODO
    auto& left = statement->terms[0];
    auto& right = statement->terms[1];

    bool b = left.var->has(right.var, context.onto);
    cout << pre << "  " << left.str << " has " << (b?"":"not") << " " << right.str << endl;
    cout << "RES " << &right.var << "   " << right.var->toString() << endl;
    if (b) { statement->state = 1; return true; }

    // DEBUG -> TODO

    auto Cconcept = context.onto->getConcept( right.var->concept ); // child concept
    auto Pconcept = context.onto->getConcept( left.var->concept ); // parent concept
    if (Cconcept == 0) { cout << "Warning (has): first concept " << right.var->concept << " not found!\n"; return false; }
    if (Pconcept == 0) { cout << "Warning (has): second concept " << left.var->concept << " not found!\n"; return false; }
    auto prop = Pconcept->getProperties( Cconcept->getName() );
    if (prop.size() == 0) cout << "Warning: has evaluation failed, property " << right.var->value << " missing!\n"; return false;
    return false;
}

// apply the statement changes to world
bool VRReasoner::apply(VRStatementPtr statement, Context& context) {
    if (statement->verb == "is") {
        auto& left = statement->terms[0];
        auto& right = statement->terms[1];
        left.var->value = right.var->value; // TOCHECK
        statement->state = 1;
        cout << pre << greenBeg << "  set " << left.str << " to " << right.str << colEnd << endl;
    }

    if (statement->verb == "has") {
        auto& left = statement->terms[0];
        auto& right = statement->terms[1];
        auto Cconcept = context.onto->getConcept( right.var->concept ); // child concept
        auto Pconcept = context.onto->getConcept( left.var->concept ); // parent concept
        if (!Pconcept || !Cconcept) { cout << "Warning: failed to apply " << statement->toString() << endl; return false; }
        auto prop = Pconcept->getProperties( Cconcept->getName() );
        if (prop.size() == 0) { cout << "Warning: failed to apply " << statement->toString() << endl; return false; }
        for (auto i : left.var->instances) i.second->add(prop[0]->getName(), right.var->value); // TODO: the first parameter is wrong
        statement->state = 1;
        cout << pre << greenBeg << "  give " << right.str << " to " << left.str << colEnd << endl;
    }

    if (statement->verb == "q") {
        string v = statement->terms[0].var->value;
        cout << "RES2 " << &context.vars[v] << "   " << context.vars[v]->toString();
        if (context.results.count(v) == 0) context.results[v] = Result();
        for (auto i : context.vars[v]->instances) context.results[v].instances.push_back(i.second);
        statement->state = 1;
        cout << pre << greenBeg << "  add result " << v << colEnd << endl;
    }

    return true;
}

bool VRReasoner::evaluate(VRStatementPtr statement, Context& context) {
    cout << pre << " " << statement->place << " eval " << statement->toString() << endl;
    statement->updateLocalVariables(context.vars, context.onto);

    if (!statement->isSimpleVerb()) { // resolve (anonymous?) variables
        string var = statement->terms[0].path.root;
        context.vars[var] = Variable::create( context.onto, statement->verb, var );
        cout << pre << blueBeg << "  added variable " << context.vars[var]->toString() << colEnd << endl;
        statement->state = 1;
        return true;
    }

    if (statement->verb == "is") return is(statement, context);
    if (statement->verb == "has") return has(statement, context);
    return false;
}
    // TODO: introduce requirements rules for the existence of some individuals

vector<Result> VRReasoner::process(string initial_query, VROntologyPtr onto) {
    cout << pre << initial_query << endl;

    Context context(onto); // create context
    context.queries.push_back(Query(initial_query));

    while( context.queries.size() ) { // while queries to process
        Query& query = context.queries.back();
        query.checkState();
        auto request = query.request;
        if (request->state == 1) {
            apply(request, context);
            cout << pre << redBeg << " solved: " << query.toString() << colEnd << endl;
            context.queries.pop_back(); continue;
        }; // query answered, pop and continue

        cout << pre << redBeg << "QUERY " << query.toString() << colEnd << endl;

        request->updateLocalVariables(context.vars, context.onto);

        for (auto& statement : query.statements) {
            if (statement->state == 1) continue;
            if (evaluate(statement, context)) {
                statement->state = 1;
            } else {
                if ( findRule(statement, context) ) continue;
                apply(statement, context);
            }
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

VRReasonerPtr VRReasoner::create() { return VRReasonerPtr( new VRReasoner() ); }


