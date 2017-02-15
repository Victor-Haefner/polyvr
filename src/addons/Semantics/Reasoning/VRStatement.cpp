#include "VRStatement.h"
#include "VRReasoner.h"
#include "core/utils/VRFunction.h"

#include <boost/bind.hpp>

using namespace OSG;

VRStatement::VRStatement() {;}

VRStatement::VRStatement(string s, int p) {
    statement = s;
    place = p;
    setup();

    store("data", &statement);
    regStorageSetupFkt( VRFunction<int>::create("statement setup", boost::bind(&VRStatement::setup, this)) );
}

void VRStatement::setup() { // parse statement
    if (statement == "") return;

    // verb
    auto s1 = VRReasoner::split(statement, '(');
    if (s1.size() == 0) return;
    verb = s1[0];

    // terms
    if (s1.size() < 2) return;
    auto s2 = VRReasoner::split(s1[1], ')');
    if (s2.size() == 0) return;
    auto s3 = VRReasoner::split( s2[0] , ',');
    terms.clear();
    for (string s : s3) terms.push_back(Term(s));
}

VRStatementPtr VRStatement::create(string s, int p) { return VRStatementPtr( new VRStatement(s,p) ); }

string VRStatement::toString() {
    string s = verb + "(";
    for (auto t : terms) s += t.path.toString() + ",";
    if (terms.size() > 0) s.pop_back();
    s += ")";
    return s;
}

void VRStatement::updateLocalVariables(map<string, VariablePtr>& globals, VROntologyPtr onto) {
    this->onto = onto;
    for (auto& t : terms) {
        if (globals.count(t.path.root)) t.var = globals[t.path.root];
        else t.var = Variable::create(onto,t.path.root);
        //cout << "updateLocalVariables " << t.str << " " << t.path.root << " " << t.var->concept << endl;
    }
}

bool VRStatement::isSimpleVerb() {
    //static string verbs[] = {"q", "is", "is_any", "is_all", "is_not", "is_not_all", "has", "has_not"};
    static string verbs[] = {"q", "is", "has"};
    for (string v : verbs) if (v == verb) return true;
    return false;
}

bool VRStatement::match(VRStatementPtr s) {
    if (terms.size() != s->terms.size()) {
        VRReasoner::print("       statement has wrong number of arguments!", VRReasoner::RED);
        return false;
    }

    for (uint i=0; i<terms.size(); i++) {
        auto tS = s->terms[i];
        auto tR = terms[i];
        if (!tS.valid() || !tR.valid()) continue; // may be anything..

        auto cS = onto->getConcept(tS.var->concept);
        auto cR = onto->getConcept(tR.var->concept);
        if (!cS || !cR) continue; // may be anything..

        if (!cS->is_a(cR) && !cR->is_a(cS)) { // check if the concepts are related
            VRReasoner::print("       var "+tR.var->value+" ("+tR.var->concept+") and var "+tS.var->value+" ("+tS.var->concept+") are not related!", VRReasoner::RED);
            return false;
        }

        //if (tS.path.nodes.size() != tR.path.nodes.size()) return false;
        /*cout << "A " << tR.var->concept << " " << tS.var->concept << endl;
        cout << "var1 " << tR.var->value << " type: " << tR.var->concept << endl;
        cout << "var2 " << tS.var->value << " type: " << tS.var->concept << endl;
        if (tR.var->concept != tS.var->concept) {
            for (auto e : tR.var->entities) {
                for (auto c : e.second->getConcepts()) {
                    cout << " A e: " << c->getName() << " c: " << tS.var->concept << endl;
                    if (c->getName() == tS.var->concept) return true;
                }
            }

            // !cR.hasParent(cS) && !cS.hasParent(cR) )
            return false;
        }*/
    }

    return true;
}
