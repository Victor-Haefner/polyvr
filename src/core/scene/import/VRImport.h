#ifndef VRIMPORT_H_INCLUDED
#define VRIMPORT_H_INCLUDED

#include <OpenSG/OSGNode.h>
#include <string>
#include "core/objects/VRObjectFwd.h"
#include "core/utils/VRUtilsFwd.h"
#include "core/utils/VRFunctionFwd.h"
#include "core/scene/VRSceneFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class NodeCore;

class VRImport {
    public:
        struct LoadJob {
            string path;
            string preset;
            string options;
            bool useBinaryCache = false;
            VRProgressPtr progress;
            VRTransformPtr res;
            VRThreadCbPtr loadCb;

            LoadJob(string p, string preset, VRTransformPtr r, VRProgressPtr pg, string opt, bool useBinaryCache);

            void load(VRThreadWeakPtr t);
        };

        struct Cache {
            VRTransformPtr root = 0;
            map<string,VRObjectPtr> objects;
            Cache();
            Cache(VRTransformPtr root);

            void setup(VRTransformPtr root);
            VRTransformPtr retrieve(VRObjectPtr parent);
        };

    private:
        map<string, Cache> cache;

        VRProgressPtr progress;
        bool ihr_flag = false; // ignore heavy ressources

        VRImport();

        void fillCache(string path, VRTransformPtr obj);
        VRTransformPtr prependTransform(VRObjectPtr o, string path);
        static void fixEmptyNames(NodeMTRecPtr o, map<string, bool>& m, string parentName = "NAN", int iChild = 0);
        static VRObjectPtr OSGConstruct(NodeMTRecPtr n, VRObjectPtr parent = 0, string name = "", string currentFile = "", NodeCore* geoTrans = 0, string geoTransName = "");

        static void osgLoad(string path, VRObjectPtr parent);

    public:
        static VRImport* get();

        VRTransformPtr load(string path, VRObjectPtr parent = 0, bool reload = false, string preset = "OSG", bool thread = false, string options = "", bool useBinaryCache = false);
        VRGeometryPtr loadGeometry(string path, string name, string preset = "OSG", bool thread = false);

        VRProgressPtr getProgressObject();
        void ingoreHeavyRessources();
};

OSG_END_NAMESPACE;

#endif // VRIMPORT_H_INCLUDED
