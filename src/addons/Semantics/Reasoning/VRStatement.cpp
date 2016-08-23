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

void VRStatement::setup() {
    if (statement == "") return;
    auto s1 = VRReasoner::split(statement, '(');
    auto v = VRReasoner::split(s1[0], '_');
    verb = v[0];
    verb_suffix = v.size() > 1 ? v[1] : "";
    auto s2 = VRReasoner::split( VRReasoner::split(s1[1], ')')[0] , ',');
    for (string s : s2) terms.push_back(Term(s));
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
    for (auto& t : terms) {
        if (globals.count(t.path.root)) t.var = globals[t.path.root];
        else t.var = Variable::create(onto,t.path.root);
        //cout << "updateLocalVariables " << t.str << " " << t.var.value << " " << t.var.concept << endl;
    }
}

bool VRStatement::isSimpleVerb() {
    //static string verbs[] = {"q", "is", "is_any", "is_all", "is_not", "is_not_all", "has", "has_not"};
    static string verbs[] = {"q", "is", "has"};
    for (string v : verbs) if (v == verb) return true;
    return false;
}

bool VRStatement::match(VRStatementPtr s) {
    for (uint i=0; i<terms.size(); i++) {
        auto tS = s->terms[i];
        auto tR = terms[i];
        if (!tS.valid() || !tR.valid()) return false;
        if (tS.path.nodes.size() != tR.path.nodes.size()) return false;

        //cout << "var1 " << tR.var.value << " type: " << tR.var.concept << endl;
        //cout << "var2 " << tS.var.value << " type: " << tS.var.concept << endl;
        if (tR.var->concept != tS.var->concept) return false;
    }
    return true;
}
