#ifndef VRIMPORT_H_INCLUDED
#define VRIMPORT_H_INCLUDED

#include <OpenSG/OSGNode.h>
#include <string>
#include "core/objects/VRObjectFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class NodeCore;

class VRImport {
    private:
        struct Cache {
            VRTransformPtr copy = 0;
            VRTransformPtr root = 0;
            map<string,VRObjectPtr> objects;
            Cache();
            Cache(VRTransformPtr root);

            VRTransformPtr retrieve();
        };
        map<string, Cache> cache;

        VRImport();

        VRTransformPtr prependTransform(VRObjectPtr o, string path);
        void fixEmptyNames(NodeRecPtr o, map<string, bool>& m, string parentName = "NAN", int iChild = 0);
        VRObjectPtr OSGConstruct(NodeRecPtr n, VRObjectPtr parent = 0, string name = "", string currentFile = "", NodeCore* geoTrans = 0, string geoTransName = "");

        void osgLoad(string path, VRObjectPtr parent);

    public:
        static VRImport* get();

        VRTransformPtr load(string path, VRObjectPtr parent = 0, bool reload = false, string preset = "OSG");
        VRGeometryPtr loadGeometry(string path, string name);
};

OSG_END_NAMESPACE;

#endif // VRIMPORT_H_INCLUDED
