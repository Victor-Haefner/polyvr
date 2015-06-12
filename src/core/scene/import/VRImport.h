#ifndef VRIMPORT_H_INCLUDED
#define VRIMPORT_H_INCLUDED

#include <OpenSG/OSGNode.h>
#include <string>

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRObject;
class VRTransform;
class VRGeometry;
class NodeCore;

class VRImport {
    private:
        struct Cache {
            VRTransform* copy = 0;
            VRTransform* root = 0;
            map<string,VRObject*> objects;
            Cache();
            Cache(VRTransform* root);

            VRTransform* retrieve();
        };
        map<string, Cache> cache;

        VRImport();

        VRTransform* prependTransform(VRObject* o, string path);
        void fixEmptyNames(NodeRecPtr o, map<string, bool>& m, string parentName = "NAN", int iChild = 0);
        VRObject* OSGConstruct(NodeRecPtr n, VRObject* parent = 0, string name = "", string currentFile = "", NodeCore* geoTrans = 0, string geoTransName = "");

    public:
        static VRImport* get();

        VRTransform* load(string path, VRObject* parent = 0, bool reload = false, string preset = "OSG");
        VRGeometry* loadGeometry(string path, string name);
};

OSG_END_NAMESPACE;

#endif // VRIMPORT_H_INCLUDED
