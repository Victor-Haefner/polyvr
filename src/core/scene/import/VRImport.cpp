#include "VRImport.h"
#include "VRCOLLADA.h"
#include "VRPLY.h"
#include "VRVTK.h"
#include "VRDXF.h"
#include "VRIFC.h"
#include "VRML.h"
#include "VRSTEPCascade.h"
#include "STEP/VRSTEP.h"
#include "E57/E57.h"
#include "GIS/VRGDAL.h"
#include "addons/Engineering/Factory/VRFactory.h"

#include <OpenSG/OSGSceneFileHandler.h>
#include <OpenSG/OSGNameAttachment.h>
#include <OpenSG/OSGComponentTransform.h>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

#include "core/objects/OSGObject.h"
#include "core/objects/object/OSGCore.h"
#include "core/objects/VRTransform.h"
#include "core/objects/VRGroup.h"
#include "core/objects/object/VRObject.h"
#include "core/objects/object/VRObjectT.h"
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

    for (uint i=0; i<o->getNChildren(); i++) fixEmptyNames(o->getChild(i), m, OSG::getName(o), i);
}

VRTransformPtr VRImport::prependTransform(VRObjectPtr o, string path) {
    if (!o) return 0;
    if (o->getChildrenCount() == 1)
        if (o->getChild(0)->getType() == "Transform")
            return static_pointer_cast<VRTransform>(o->getChild(0));

    boost::filesystem::path p(path);
    auto trans = VRTransform::create( p.filename().string() );
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
    cout << " done " << endl;
}

int fileSize(string path) {
    ifstream in(path, ios::binary | ios::ate);
    int L = in.tellg();
    in.close();
    return L;
}

VRTransformPtr VRImport::load(string path, VRObjectPtr parent, bool reload, string preset, bool thread, string options, bool useBinaryCache) {
    cout << "VRImport::load " << path << " " << preset << endl;
    if (ihr_flag) if (fileSize(path) > 3e7) return 0;
    setlocale(LC_ALL, "C");

    // check cache
    reload = reload ? true : (cache.count(path) == 0);
    if (!reload) {
        auto res = cache[path].retrieve(parent);
        cout << "load " << path << " : " << res << " from cache\n";
        return res;
    }

    // check file path
    if (!exists(path)) { cout << "VRImport::load " << path << " not found!" << endl; return 0; }

    VRTransformPtr res = VRTransform::create("proxy");
    if (!thread) {
        LoadJob job(path, preset, res, progress, options, useBinaryCache);
        job.load(VRThreadWeakPtr());
        return cache[path].retrieve(parent);
    } else {
        fillCache(path, res);
        auto r = cache[path].retrieve(parent);
        auto job = new LoadJob(path, preset, r, progress, options, useBinaryCache); // TODO: fix memory leak!
        job->loadCb = VRFunction< VRThreadWeakPtr >::create( "geo load", boost::bind(&LoadJob::load, job, _1) );
        VRScene::getCurrent()->initThread(job->loadCb, "geo load thread", false, 1);
        return r;
    }
}

VRImport::LoadJob::LoadJob(string p, string pr, VRTransformPtr r, VRProgressPtr pg, string opt, bool ubc) {
    path = p;
    res = r;
    progress = pg;
    preset = pr;
    options = opt;
    useBinaryCache = ubc;
}

void VRImport::LoadJob::load(VRThreadWeakPtr tw) {
    VRThreadPtr t = tw.lock();

    bool thread = false;
    if (t) { t->syncFromMain(); thread = true; }

    auto loadSwitch = [&]() {
        auto bpath = boost::filesystem::path(path);
        string ext = bpath.extension().string();
        cout << "load " << path << " ext: " << ext << " preset: " << preset << "\n";
        if (ext == ".e57") { loadE57(path, res); return; }
        if (ext == ".xyz") { loadXYZ(path, res); return; }
        if (ext == ".ply") { loadPly(path, res); return; }
        //if (ext == ".step" || ext == ".stp" || ext == ".STEP" || ext == ".STP") { VRSTEP step; step.load(path, res, options); return; }
#ifdef WITH_STEP
        if (ext == ".step" || ext == ".stp" || ext == ".STEP" || ext == ".STP") { loadSTEPCascade(path, res); return; }
#endif
        if (ext == ".wrl" && preset == "SOLIDWORKS-VRML2") { VRFactory f; if (f.loadVRML(path, progress, res, thread)) return; else preset = "OSG"; }
        if (ext == ".wrl" && preset == "PVR") { loadVRML(path, res, progress, thread); return; }
        if (ext == ".vtk") { loadVtk(path, res); return; }
        if (ext == ".shp") { loadSHP(path, res); return; }
        if (ext == ".pdf") { loadPDF(path, res); return; }
        if (ext == ".tiff" || ext == ".tif") { loadTIFF(path, res); return; }
        if (ext == ".hgt") { loadTIFF(path, res); return; }
        if (ext == ".dxf") { loadDXF(path, res); return; }
#ifndef NO_IFC
        if (ext == ".ifc") { loadIFC(path, res); return; }
#endif
        if (preset == "OSG" || preset == "COLLADA") osgLoad(path, res);
        if (preset == "COLLADA") loadCollada(path, res);
    };

    string osbPath = getFolderName(path) + "/." + getFileName(path) + ".osb";
    bool loadedFromCache = false;
    if (useBinaryCache && exists(osbPath)) {
        // TODO: create descriptive hash of file, load hash and compare
        osgLoad(osbPath, res);
        loadedFromCache = true;
    } else loadSwitch();

    VRImport::get()->fillCache(path, res);
    if (t) t->syncToMain();

    if (useBinaryCache && !loadedFromCache && res->getChild(0)) {
        for (auto c : res->getChildren(true)) { if (auto t = dynamic_pointer_cast<VRTransform>(c)) t->enableOptimization(false); }
        string osbPath = getFolderName(path) + "/." + getFileName(path) + ".osb";
        SceneFileHandler::the()->write(res->getChild(0)->getNode()->node, osbPath.c_str());
        for (auto c : res->getChildren(true)) { if (auto t = dynamic_pointer_cast<VRTransform>(c)) t->enableOptimization(true); }
        // TODO: create descriptive hash of file, store hash
    }
}

string repSpaces(string s) {
    for(auto it = s.begin(); it != s.end(); ++it) {
        if(*it == ' ') *it = '|';
    }
    return s;
}

string unrepSpaces(string s) {
    for(auto it = s.begin(); it != s.end(); ++it) {
        if(*it == '|') *it = ' ';
    }
    return s;
}

VRObjectPtr VRImport::OSGConstruct(NodeMTRecPtr n, VRObjectPtr parent, string name, string currentFile, NodeCore* geoTrans, string geoTransName) {
    if (n == 0) return 0; // TODO add an osg wrap method for each object?

    VRObjectPtr tmp = 0;
    VRMaterialPtr tmp_m;
    VRGeometryPtr tmp_g;
    VRTransformPtr tmp_e;
    VRGroupPtr tmp_gr;

    NodeCoreMTRecPtr core = n->getCore();
    string t_name = core->getTypeName();

    if (getName(n)) name = getName(n);
    else name = "Unnamed";
    if (name == "") name = "NAN";

    if (t_name == "Group") {//OpenSG Group
        tmp = VRObject::create(name);
        tmp->setCore(OSGCore::create(core), "Object");
        tmp->addAttachment("collada_name", name);
    }

    else if (t_name == "ComponentTransform") {
        if (tmp == 0) {
            tmp_e = VRTransform::create(name);
            tmp_e->setMatrix(toMatrix4d(dynamic_cast<ComponentTransform *>(n->getCore())->getMatrix()));
            tmp = tmp_e;
        }
    }

    else if (t_name == "Transform") {
        if (n->getNChildren() == 1) { // try to optimize the tree by avoiding obsolete transforms
            string tp = n->getChild(0)->getCore()->getTypeName();
            if (tp == "Geometry") {
                geoTrans = n->getCore();
                geoTransName = name;
                tmp = parent;
            }
        }

        if (tmp == 0) {
            tmp_e = VRTransform::create(name);
            tmp_e->setMatrix(toMatrix4d(dynamic_cast<Transform *>(n->getCore())->getMatrix()));
            tmp = tmp_e;
            tmp->addAttachment("collada_name", name);
        }
    }

    else if (t_name == "MaterialGroup") { // highly inefficient! is there a reason to support this??
        tmp = parent;
        /*tmp_m = VRMaterial::create(name);
        tmp = tmp_m;
        tmp->setCore(OSGCore::create(core), "Material");*/
    }

    else if (t_name == "Geometry") {
        auto osgGeo = dynamic_cast<Geometry*>(n->getCore());
        if (!osgGeo->getPositions()) return 0;
        if (osgGeo->getPositions()->size() == 0) return 0;
        tmp_g = VRGeometry::create(name);
        if (geoTrans) {
            tmp_g->addAttachment("collada_name", geoTransName);
            tmp_g->setMatrix(toMatrix4d(dynamic_cast<Transform*>(geoTrans)->getMatrix()));
            geoTrans = 0;
            geoTransName = "";
        }

        VRGeometry::Reference ref;
        ref.type = VRGeometry::FILE;
        ref.parameter = repSpaces(currentFile) + " " + repSpaces( tmp_g->getBaseName() );
        tmp_g->setMesh( OSGGeometry::create( osgGeo ), ref, true);
        tmp = tmp_g;
    }

    else {
        tmp = VRObject::create(name);
        tmp->setCore(OSGCore::create(core), t_name);
    }

    for (uint i=0;i<n->getNChildren();i++) {
        auto obj = OSGConstruct(n->getChild(i), tmp, name, currentFile, geoTrans, geoTransName);
        if (obj) tmp->addChild(obj);
    }

    return tmp;
}

VRGeometryPtr VRImport::loadGeometry(string file, string object, string preset, bool thread) {
    file = unrepSpaces(file);
    object = unrepSpaces(object);

    if (cache.count(file) == 0) load(file, 0, false, preset, thread);

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
    //for (auto c : root->getChildren(true)) objects[getName(c->getNode()->node)] = c;
    for (auto c : root->getChildren(true)) objects[c->getName()] = c;
    //for (auto c : root->getChildren(true)) objects[c->getBaseName()] = c; // not a valid option as basename is not unique! SW VRML import does not provide unique names

    root->setNameSpace("VRImportCache");
    for (auto o : root->getChildren(true) ) o->setNameSpace("VRImportCache");
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



