#ifndef VRAMLLOADER_H_INCLUDED
#define VRAMLLOADER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>

#include "core/objects/VRObjectFwd.h"
#include "core/utils/VRUtilsFwd.h"
#include "addons/Engineering/VREngineeringFwd.h"
#include "addons/Semantics/VRSemanticsFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRAMLLoader {
    private:
        VROntologyPtr ontology;
        VRTransformPtr scene;

        void processElement(XMLElementPtr node, VRObjectPtr obj);
        void readNode(XMLElementPtr node, VRObjectPtr obj);

    public:
        VRAMLLoader();
        ~VRAMLLoader();

        static VRAMLLoaderPtr create();

        void read(string path);
        void write(string path);

        VROntologyPtr getOntology();
        VRTransformPtr getScene();
};

OSG_END_NAMESPACE;

#endif // VRAMLLOADER_H_INCLUDED
