#ifndef VRSTATEMENT_H_INCLUDED
#define VRSTATEMENT_H_INCLUDED

#include <string>
#include <vector>
#include <map>

#include "VRSemanticUtils.h"
#include "core/utils/VRStorage.h"

using namespace std;

OSG_BEGIN_NAMESPACE;

struct VRStatement : VRStorage {
    string statement;
    string verb;
    string verb_suffix;
    vector<Term> terms;
    int state = 0;
    int place = -1;
    VROntologyPtr onto;

    VRStatement();
    VRStatement(string s, int i = -1);
    static VRStatementPtr create(string s = "", int i = -1);
    void setup();

    string toString();
    void updateLocalVariables(map<string, VariablePtr>& globals, VROntologyPtr onto);
    bool isSimpleVerb();
    bool match(VRStatementPtr s);
};

OSG_END_NAMESPACE;

#endif // VRSTATEMENT_H_INCLUDED
