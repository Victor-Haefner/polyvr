#include "VRImport.h"
#include "VRExport.h"

#ifndef WITHOUT_COLLADA
#include "COLLADA/VRCOLLADA.h"
#endif
#include "VRPLY.h"
#include "VRTS.h"
#ifndef WASM
#ifndef WITHOUT_VTK
#include "VRVTK.h"
#endif
#include "VRDXF.h"
#include "VRDWG.h"
#include "VRIFC.h"
#endif
#include "VRML.h"
#ifndef WITHOUT_STEP
#include "VRSTEPCascade.h"
#endif
#ifndef WITHOUT_STEPCODE
#include "STEP/VRSTEP.h"
#endif
#ifndef WITHOUT_E57
#include "E57/E57.h"
#endif
#ifndef WITHOUT_GDAL
#include "GIS/VRGDAL.h"
#endif
#include "GLTF/GLTF.h"
#include "addons/Engineering/Factory/VRFactory.h"

#include <OpenSG/OSGSceneFileHandler.h>
#include <OpenSG/OSGNameAttachment.h>
#include <OpenSG/OSGComponentTransform.h>
#include <OpenSG/OSGDistanceLOD.h>

#include "core/objects/OSGObject.h"
#include "core/objects/object/OSGCore.h"
#include "core/objects/VRTransform.h"
#include "core/objects/VRGroup.h"
#include "core/objects/object/VRObject.h"
#include "core/objects/object/VRObjectT.h"
#include "core/objects/VRLod.h"
#include "core/objects/VRPointCloud.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/OSGGeometry.h"
#include "core/objects/material/VRMaterial.h"
#include "core/utils/VRProgress.h"
#include "core/utils/VRFunction.h"
#include "core/utils/toString.h"
#include "core/utils/system/VRSystem.h"
#include "core/scene/VRScene.h"

OSG_BEGIN_NAMESPACE;

VRImport::VRImport() {
    progress = VRProgress::create();
}

VRImport* VRImport::get() {
    static VRImport* s = new VRImport();
    return s;
}

void VRImport::analyze(string path, string out) {
    string ext = getFileExtension(path);
#ifndef WITHOUT_GDAL
    if (ext == ".shp") analyzeSHP(path, out);
#endif
}

void VRImport::fixEmptyNames(NodeMTRecPtr o, map<string, bool>& m, string parentName, int iChild) {
    if (!OSG::getName(o)) OSG::setName(o, parentName);

    // make unique
    string name = getName(o);
    string orig = name;
    for (int i=0; m.count(name); i++) {
        stringstream ss; ss << orig << "." << i;
        name = ss.str();
    }
    setName(o, name.c_str());
    m[name] = true;

    for (unsigned int i=0; i<o->getNChildren(); i++) fixEmptyNames(o->getChild(i), m, OSG::getName(o), i);
}

VRTransformPtr VRImport::prependTransform(VRObjectPtr o, string path) {
    if (!o) return 0;
    if (o->getChildrenCount() == 1)
        if (o->getChild(0)->getType() == "Transform")
            return static_pointer_cast<VRTransform>(o->getChild(0));

    auto trans = VRTransform::create( getFileName(path,0) );
    trans->addChild(o);
    return trans;
}

void VRImport::osgLoad(string path, VRObjectPtr res) {
    cout << "OSG Load " << path << endl;
    res->setName(path);
    NodeMTRecPtr n = SceneFileHandler::the()->read(path.c_str());
    if (n == 0) return;
    map<string, bool> m;
    fixEmptyNames(n,m);
    auto obj = OSGConstruct(n, res, path, path);
    if (obj) res->addChild( obj );

    // fix geo references
    string ns = "tmp_"+genUUID();
    res->setNameSpace(ns); // maybe try a unique namespace (use the path)?
    for (auto o : res->getChildren(true) ) o->setNameSpace(ns);
    for (auto o : res->getChildren(true) ) {
        auto geo = dynamic_pointer_cast<VRGeometry>(o);
        if (geo) {
            VRGeometry::Reference ref;
            ref.type = VRGeometry::FILE;
            ref.parameter = path + "|" + geo->getName();
            geo->setReference(ref);
        }
    }
    res->resetNameSpace();
    for (auto o : res->getChildren(true) ) o->resetNameSpace();

    cout << " done " << endl;
}

int fileSize(string path) {
    ifstream in(path, ios::binary | ios::ate);
    int L = in.tellg();
    in.close();
    return L;
}

/// --------------------------------------------------

void testSync(int t) {
    auto testThread = VRScene::getCurrent()->getThread(t);
    VRScene::getCurrent()->setupThreadState(testThread);
    VRScene::getCurrent()->importThreadState(testThread);
    VRScene::getCurrent()->waitThread(t);
}

/// --------------------------------------------------
void VRImport::addPath(string folder) {
    importPaths.push_back(folder);
}

bool VRImport::checkPath(string& path) {
    if (exists(path)) return true;
    for (auto& folder : importPaths) {
        if (exists(folder+"/"+path)) {
            path = folder+"/"+path;
            return true;
        }
    }
    return false;
}

VRTransformPtr VRImport::load(string path, VRObjectPtr parent, bool useCache, string preset, bool thread, map<string, string> options, bool useBinaryCache) {
    // check file path
    if (!checkPath(path)) { cout << "VRImport::load " << path << " not found!" << endl; return 0; }

    cout << "VRImport::load " << path << ", with preset: " << preset << ", useCache: " << useCache << endl;
    if (ihr_flag) if (fileSize(path) > 3e7) return 0;
    setlocale(LC_ALL, "en_US.UTF-8"); // if necessary call with utf8 instead (see polyvr.cpp)

    // check cache
    if (useCache && cache.count(path)) {
        auto res = cache[path].retrieve(parent);
        cout << "load " << path << " : " << res << " from cache\n";
        return res;
    }

    VRTransformPtr res = VRTransform::create( getFileName(path, false) );
    if (!thread) {
        LoadJob job(path, preset, res, progress, options, useCache, useBinaryCache);
        job.load(VRThreadWeakPtr());
        if (!useCache && parent) parent->addChild(res);
        if (useCache) return cache[path].retrieve(parent);
        else return res;
    } else {
        if (!useCache && parent) parent->addChild(res);
        if (useCache) {
            fillCache(path, res);
            res = cache[path].retrieve(parent);
        }

        auto job = new LoadJob(path, preset, res, progress, options, useCache, useBinaryCache); // TODO: fix memory leak!
        job->loadCb = VRThreadCb::create( "geo load", bind(&LoadJob::load, job, _1) );
        /*auto t =*/ VRScene::getCurrent()->initThread(job->loadCb, "geo load thread", false, 1);
        //testSync(t);
        return res;
    }
}

void VRImport::clearCache() {
    cout << " - - - - - - clearCache " << endl;
    cache.clear();
}

void VRImport::addEventCallback(VRImportCbPtr cb) {
    callbacks.push_back(cb);
}

void VRImport::remEventCallback(VRImportCbPtr cb) {
    callbacks.erase(std::remove(callbacks.begin(), callbacks.end(), cb), callbacks.end());
}

void VRImport::triggerCallbacks(const VRImportJob& params) {
    auto doTrigger = [&](VRImportJob params) {
        for (auto cb : callbacks) (*cb)(params);
    };

    auto cb = VRUpdateCb::create("trigger import events", bind(doTrigger, params));
    VRScene::getCurrent()->queueJob(cb);
}

VRImport::LoadJob::LoadJob(string p, string pr, VRTransformPtr r, VRProgressPtr pg, map<string, string> opt, bool uc, bool ubc) {
    params.path = p;
    params.res = r;
    params.progress = pg;
    params.preset = pr;
    params.options = opt;
    params.useCache = uc;
    params.useBinaryCache = ubc;
}

void VRImport::LoadJob::load(VRThreadWeakPtr tw) {
    VRThreadPtr t = tw.lock();

    bool thread = false;
    if (t) { t->syncFromMain(); thread = true; }

    string& path = params.path;
    auto& res = params.res;
    auto& progress = params.progress;
    auto& preset = params.preset;
    auto& options = params.options;
    auto& useCache = params.useCache;
    auto& useBinaryCache = params.useBinaryCache;

    auto loadSwitch = [&]() {
        string ext = getFileExtension(path);

        auto clist = Thread::getCurrentChangeList();
        int Ncr0 = clist->getNumCreated();
        int Nch0 = clist->getNumChanged();
        cout << "load " << path << " ext: " << ext << " preset: " << preset << ", until now created: " << Ncr0 << ", changed: " << Nch0 << endl;
#ifndef WITHOUT_E57
        if (ext == ".e57") { loadE57(path, res, options); return; }
        if (ext == ".pcb") { loadPCB(path, res, options); return; }
        if (ext == ".xyz") { loadXYZ(path, res, options); return; }
#endif // TODO: move loadPCB and loadXYZ from E57 include
        if (ext == ".ply") { loadPly(path, res); return; }
        if (ext == ".ts") { loadTS(path, res, options); return; }
        if (ext == ".step" || ext == ".stp" || ext == ".STEP" || ext == ".STP") {
            if (preset == "PVR") {
#ifndef WITHOUT_STEPCODE
                VRSTEPPtr step = VRSTEP::create();
                step->load(path, res, options, progress, thread);
#endif
            } else {
#ifndef WITHOUT_STEP
				loadSTEPCascade(path, res, options);
#endif
			}
            return;
        }
#ifndef WITHOUT_IFC
		if (ext == ".ifc") { loadIFC(path, res); return; }
#endif
        if (preset == "PVR" || preset == "SOLIDWORKS-VRML2") {
            if (ext != ".wrl") preset = "OSG";
            if (ext == ".wrl" && preset == "SOLIDWORKS-VRML2") { VRFactory f; if (f.loadVRML(path, progress, res, thread)); else preset = "OSG"; }
            if (ext == ".wrl" && preset == "PVR") { loadVRML(path, res, progress, thread); }
        }
#ifndef WASM
#ifndef WITHOUT_VTK
        if (ext == ".vtk") { loadVTK(path, res); return; }
        if (ext == ".gz" && endsWith(path, ".vtk.gz")) { loadVTK(path, res); return; }
#endif
#ifndef WITHOUT_GDAL
        if (ext == ".pdf") { loadPDF(path, res, options); return; }
        if (ext == ".shp") { loadSHP(path, res, options); return; }
        if (ext == ".tiff" || ext == ".tif") { loadTIFF(path, res, options); return; }
        if (ext == ".hgt") { loadTIFF(path, res, options); return; }
#endif
        if (preset == "DXF") {
            if (ext == ".dxf" || ext == ".DXF") {
                loadDXF(path, res);
                return;
            }
        }
#ifndef WITHOUT_DWG
        if (ext == ".dwg" || ext == ".dxf" || ext == ".DWG" || ext == ".DXF") {
            loadDWG(path, res, options);
            return;
        }
#endif
#endif
        if (ext == ".gltf" || ext == ".glb") { loadGLTF(path, res, progress, thread); return; }
        if (ext == ".osb" || ext == ".osg") { osgLoad(path, res); return; }
        if (preset == "OSG") osgLoad(path, res); // fallback
#ifndef WITHOUT_COLLADA
        if (preset == "COLLADA") loadCollada(path, res, options);
#endif
        cout << " additional created: " << clist->getNumCreated()-Ncr0 << ", changed: " << clist->getNumChanged()-Nch0 << endl;
    };

    string osbPath = getFolderName(path) + "/." + getFileName(path) + ".osb";
    bool loadedFromCache = false;
    if (useBinaryCache) cout << "check for cache " << osbPath << " -> exist? " << exists(osbPath) << endl;
    if (useBinaryCache && exists(osbPath)) {
        // TODO: create descriptive hash of file, load hash and compare
        cout << "load from binary cache" << endl;
        osgLoad(osbPath, res);
        loadedFromCache = true;
    } else loadSwitch();

    if (!t && useCache) VRImport::get()->fillCache(path, res);
    if (t) t->syncToMain();

    if (useBinaryCache && !loadedFromCache && res->getChild(0)) {
        for (auto c : res->getChildren(true)) { if (auto t = dynamic_pointer_cast<VRTransform>(c)) t->enableOptimization(false); }
        string osbPath = getFolderName(path) + "/." + getFileName(path) + ".osb";
        //SceneFileHandler::the()->write(res->getChild(0)->getNode()->node, osbPath.c_str());
        VRExport::get()->write(res->getChild(0), osbPath);
        for (auto c : res->getChildren(true)) { if (auto t = dynamic_pointer_cast<VRTransform>(c)) t->enableOptimization(true); }
        // TODO: create descriptive hash of file, store hash
        cout << "store in binary cache: " << path << " " << osbPath << endl;
    }

    VRImport::get()->triggerCallbacks(params);
}

VRObjectPtr VRImport::OSGConstruct(NodeMTRecPtr n, VRObjectPtr parent, string name, string currentFile, NodeMTRecPtr geoTrans, NodeMTRecPtr geoObj, string geoTransName) {
    if (n == 0) return 0;
    VRObjectPtr tmp = 0;
    NodeCoreMTRecPtr core = n->getCore();
    string t_name = core->getTypeName();
    name = getName(n) ? getName(n) : "Unnamed";
    if (name == "") name = "NAN";

    auto wrapGroup = [](string& name, NodeMTRecPtr& n) {
        auto tmp = VRObject::create(name);
        tmp->wrapOSG(OSGObject::create(n));
        return tmp;
    };

    auto wrapTransform = [](string& name, NodeMTRecPtr& n) {
        auto tmp = VRTransform::create(name);
        tmp->wrapOSG(OSGObject::create(n));
        return tmp;
    };

    auto wrapLoD = [](string& name, NodeMTRecPtr& n) {
        auto tmp = VRLod::create(name);
        tmp->wrapOSG(OSGObject::create(n));
        return tmp;
    };

    auto wrapPointcloud = [](string& name, NodeMTRecPtr& n) {
        auto tmp = VRPointCloud::create(name);
        tmp->wrapOSG(OSGObject::create(n));
        return tmp;
    };

    auto wrapGeometry = [&](string& name, NodeMTRecPtr& n) -> VRGeometryPtr { // TODO: fix wrapOSG for this case!
        auto osgGeo = dynamic_cast<Geometry*>(n->getCore());
        if (!osgGeo->getPositions()) return 0;
        if (osgGeo->getPositions()->size() == 0) return 0;

        auto tmp = VRGeometry::create(name);
        //tmp->wrapOSG(OSGObject::create(n));

        VRGeometry::Reference ref;
        ref.type = VRGeometry::FILE;
        ref.parameter = currentFile + "|" + tmp->getName();
        tmp->setReference(ref);

        tmp->setMesh( OSGGeometry::create( osgGeo ), ref, true);

        auto p = n->getParent();
        p->subChild(n);
        p->addChild(tmp->getNode()->node);

        return tmp;
    };

    auto wrapGeometry2 = [&](string& name, NodeMTRecPtr& n, NodeMTRecPtr& nGeo) -> VRGeometryPtr {
        auto osgGeo = dynamic_cast<Geometry*>(nGeo->getCore());
        if (!osgGeo->getPositions()) return 0;
        if (osgGeo->getPositions()->size() == 0) return 0;

        auto tmp = VRGeometry::create(name);
        tmp->wrapOSG(OSGObject::create(n), OSGObject::create(nGeo));

        VRGeometry::Reference ref;
        ref.type = VRGeometry::FILE;
        ref.parameter = currentFile + "|" + tmp->getName();
        tmp->setReference(ref);

        return tmp;
    };

    auto hasSingleGeometry = [](NodeMTRecPtr n) -> NodeMTRecPtr {
        NodeMTRecPtr nGeo;
        int N = 0;
        for (unsigned int i=0; i<n->getNChildren(); i++) {
            NodeMTRecPtr c = n->getChild(i);
            string tp = c->getCore()->getTypeName();
            if (tp == "Geometry") {
                nGeo = c;
                N++;
            }
        }
        if (N == 1) return nGeo;
        return 0;
    };

    NodeMTRecPtr childToSkip = 0;
    vector<NodeMTRecPtr> children;
    for (unsigned int i=0; i<n->getNChildren(); i++) children.push_back(n->getChild(i));

    if (t_name == "Group" || t_name == "Transform" || t_name == "ComponentTransform") { // resolve special case transform->geometry
        if (NodeMTRecPtr nGeo = hasSingleGeometry(n)) {
            tmp = wrapGeometry2(name, n, nGeo);
            if (tmp) childToSkip = nGeo;
        }
    }

    if (!tmp) {
        if (t_name == "Group") tmp = wrapGroup(name, n);
        if (t_name == "Transform") tmp = wrapTransform(name, n);
        if (t_name == "ComponentTransform") tmp = wrapTransform(name, n);
        if (t_name == "MaterialGroup") tmp = wrapGroup(name, n);
        if (t_name == "DistanceLOD") tmp = wrapLoD(name, n);
        if (t_name == "PointCloud") tmp = wrapPointcloud(name, n);
        if (t_name == "Geometry") tmp = wrapGeometry(name, n);
    }

    if (!tmp) tmp = wrapGroup(name, n);

    for (auto& child : children) {
        if (child == childToSkip) continue;
        auto obj = OSGConstruct(child, tmp, name, currentFile, geoTrans, geoObj, geoTransName);
        if (obj) tmp->addChild(obj, false);
    }

    tmp->addAttachment("collada_name", name);
    return tmp;
}

VRGeometryPtr VRImport::loadGeometry(string file, string object, string preset, bool thread) {
    if (cache.count(file) == 0) load(file, 0, true, preset, thread); // set useCache to true, else this wont work obviously..

    if (cache.count(file) == 0) {
        cout << "VRImport::loadGeometry - Warning: " << file << " not in cache" << endl;
        return 0;
    }

    if (cache[file].objects.count(object) == 0) {
        cout << "VRImport::loadGeometry - Warning: " << file << " in cache but has no object " << object << endl;
        cout << " cache root: " << cache[file].root->getName() << " cache size: " << cache[file].objects.size() << endl;
        //for (auto o : cache[file].root->getChildren(true)) cout << " cache " << o->getName() << endl;
        for (auto o : cache[file].objects) cout << " cache " << o.first << endl;
        return 0;
    }

    VRObjectPtr o = cache[file].objects[object];
    if (o->getType() != "Geometry") {
        cout << "VRImport::loadGeometry - Warning: " << file << " is cached but object " << object << " has wrong type: " << o->getType() << endl;
        for (auto o : cache[file].objects) cout << " cache " << o.first << ", " << o.second->getType() << endl;
        return 0;
    }

    return static_pointer_cast<VRGeometry>(o);
}

VRProgressPtr VRImport::getProgressObject() { return progress; }

void VRImport::ingoreHeavyRessources() { ihr_flag = true; }

VRImport::Cache::Cache() {;}
VRImport::Cache::Cache(VRTransformPtr root) { setup(root); }

void VRImport::Cache::setup(VRTransformPtr root) {
    objects.clear();
    this->root = root;

    string ns = "VRImportCache_"+genUUID();

    root->setNameSpace(ns); // maybe try a unique namespace (use the path)?
    for (auto o : root->getChildren(true) ) o->setNameSpace(ns);

    //for (auto c : root->getChildren(true)) objects[getName(c->getNode()->node)] = c;
    for (auto c : root->getChildren(true)) objects[c->getName()] = c;
    //for (auto c : root->getChildren(true)) objects[c->getBaseName()] = c; // not a valid option as basename is not unique! SW VRML import does not provide unique names
}

VRTransformPtr VRImport::Cache::retrieve(VRObjectPtr parent) {
    auto res = static_pointer_cast<VRTransform>(root->duplicate());
    res->resetNameSpace();
    for (auto o : res->getChildren(true) ) o->resetNameSpace();

    if (parent) parent->addChild(res);
    return res;
}


void VRImport::fillCache(string path, VRTransformPtr obj) {
    if (cache.count(path) == 0) cache[path] = Cache();
    cache[path].setup(obj);
    cout << "\nVRImport::fillCache " << path << ", cache size: " << cache[path].objects.size() << endl;
}



OSG_END_NAMESPACE;
