#ifndef VRONTOLOGYRULE_H_INCLUDED
#define VRONTOLOGYRULE_H_INCLUDED

#include "VROntologyUtils.h"
#include "addons/Semantics/VRSemanticsFwd.h"
#include "core/utils/VRName.h"

using namespace std;
namespace OSG {

struct VROntologyRule : public VROntoID, public VRName {
    string rule;
    string associatedConcept;

    VROntologyRule(string rule, string associatedConcept);

    static VROntologyRulePtr create(string rule = "", string associatedConcept = "");
    string toString();

    VRStatementPtr query;
    vector<VRStatementPtr> statements;

    VRStatementPtr addStatement(string name);
    VRStatementPtr getStatement(int i);
    void remStatement(VRStatementPtr s);
};

}

#endif
