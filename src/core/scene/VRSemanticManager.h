#ifndef VRSEMANTICMANAGER_H_INCLUDED
#define VRSEMANTICMANAGER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>
#include <map>
#include <vector>
#include "addons/Semantics/VRSemanticsFwd.h"
#include "VRSceneFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRSemanticManager {
    private:
        map<string, VROntologyPtr> ontologies;

    public:
        VRSemanticManager();
        ~VRSemanticManager();

        static VRSemanticManagerPtr create();

        VROntologyPtr addOntology(string name);
        VROntologyPtr loadOntology(string path);
        VROntologyPtr getOntology(string name);

        void renameOntology(string name, string new_name);

        vector<VROntologyPtr> getOntologies();
};

OSG_END_NAMESPACE;

#endif // VRSEMANTICMANAGER_H_INCLUDED
