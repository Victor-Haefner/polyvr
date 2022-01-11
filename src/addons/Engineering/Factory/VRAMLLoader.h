#ifndef VRAMLLOADER_H_INCLUDED
#define VRAMLLOADER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>
#include <map>

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
        string AMLDir;
        map<string, VRObjectPtr> assets;

        void processElement(XMLElementPtr node, VRTransformPtr obj);
        void readNode(XMLElementPtr node, VRTransformPtr obj);

        void writeHeader(ofstream& stream, string fileName);
        void writeFooter(ofstream& stream);
        void writeScene(ofstream& stream, string daeFolder);
        void writeOntology(ofstream& stream);

    public:
        VRAMLLoader();
        ~VRAMLLoader();

        static VRAMLLoaderPtr create();

        void read(string path);
        void write(string path);

        VROntologyPtr getOntology();
        VRTransformPtr getScene();

        string addAsset(VRObjectPtr obj);
};

OSG_END_NAMESPACE;

#endif // VRAMLLOADER_H_INCLUDED
