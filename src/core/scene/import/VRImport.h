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


struct VRImportJob {
    string path;
    string preset;
    map<string, string> options;
    bool useCache = false;
    bool useBinaryCache = false;
    VRProgressPtr progress;
    VRTransformPtr res;
};

ptrFctFwd( VRImport, VRImportJob );

class VRImport {
    public:
        struct LoadJob {
            VRImportJob params;
            VRThreadCbPtr loadCb;

            LoadJob(string p, string preset, VRTransformPtr r, VRProgressPtr pg, map<string, string> opt, bool useCache, bool useBinaryCache);
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
        vector<VRImportCbPtr> callbacks;
        vector<string> importPaths;

        VRProgressPtr progress;
        bool ihr_flag = false; // ignore heavy ressources

        VRImport();

        void fillCache(string path, VRTransformPtr obj);
        VRTransformPtr prependTransform(VRObjectPtr o, string path);
        static void fixEmptyNames(NodeMTRecPtr o, map<string, bool>& m, string parentName = "NAN", int iChild = 0);
        static void osgLoad(string path, VRObjectPtr parent);

    public:
        static VRObjectPtr OSGConstruct(NodeMTRecPtr n, VRObjectPtr parent = 0, string name = "", string currentFile = "", NodeMTRecPtr geoTrans = 0, NodeMTRecPtr geoObj = 0, string geoTransName = "");

    public:
        static VRImport* get();
        void clearCache();

        void addPath(string folder);
        bool checkPath(string& path);

        void analyze(string path, string out);
        VRTransformPtr load(string path, VRObjectPtr parent = 0, bool useCache = true, string preset = "OSG", bool thread = false, map<string, string> options = map<string, string>(), bool useBinaryCache = false);
        VRGeometryPtr loadGeometry(string path, string name, string preset = "OSG", bool thread = false);

        VRProgressPtr getProgressObject();
        void ingoreHeavyRessources();

        void addEventCallback(VRImportCbPtr cb);
        void remEventCallback(VRImportCbPtr cb);
        void triggerCallbacks(const VRImportJob& params);
};

OSG_END_NAMESPACE;

#endif // VRIMPORT_H_INCLUDED
