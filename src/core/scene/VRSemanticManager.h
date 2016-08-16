#ifndef VRSEMANTICMANAGER_H_INCLUDED
#define VRSEMANTICMANAGER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>
#include <map>
#include <vector>
#include "addons/Semantics/VRSemanticsFwd.h"
#include "VRSceneFwd.h"
#include "core/utils/VRStorage.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRSemanticManager : public VRStorage {
    private:
        map<string, VROntologyPtr> ontologies;

    public:
        VRSemanticManager();
        ~VRSemanticManager();

        static VRSemanticManagerPtr create();

        VROntologyPtr addOntology(string name);
        VROntologyPtr loadOntology(string path);
        VROntologyPtr getOntology(string name);
        void remOntology(VROntologyPtr o);
        void renameOntology(string name, string new_name);
        vector<VROntologyPtr> getOntologies();

        void update();
};

OSG_END_NAMESPACE;

#endif // VRSEMANTICMANAGER_H_INCLUDED
