#ifndef VRSTATEMENT_H_INCLUDED
#define VRSTATEMENT_H_INCLUDED

#include <string>
#include <vector>
#include <map>

#include "VRSemanticUtils.h"

using namespace std;

OSG_BEGIN_NAMESPACE;

struct VRStatement {
    string verb;
    string verb_suffix;
    vector<Term> terms;
    int state = 0;
    int place = -1;

    VRStatement();
    VRStatement(string s, int i = -1);
    static VRStatementPtr New(string s, int i = -1);

    string toString();
    void updateLocalVariables(map<string, VariablePtr>& globals, VROntologyPtr onto);
    bool isSimpleVerb();
    bool match(VRStatementPtr s);
};

OSG_END_NAMESPACE;

#endif // VRSTATEMENT_H_INCLUDED
