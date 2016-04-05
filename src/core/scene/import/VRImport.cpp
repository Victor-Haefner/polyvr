#include "VRImport.h"
#include "VRCOLLADA.h"
#include "VRPLY.h"
#include "STEP/VRSTEP.h"
#include "E57/E57.h"
#include "addons/Engineering/Factory/VRFactory.h"

#include <OpenSG/OSGSceneFileHandler.h>
#include <OpenSG/OSGNameAttachment.h>
#include <OpenSG/OSGComponentTransform.h>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

#include "core/objects/VRTransform.h"
#include "core/objects/VRGroup.h"
#include "core/objects/object/VRObject.h"
#include "core/objects/object/VRObjectT.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/material/VRMaterial.h"
#include "core/utils/VRProgress.h"
#include "core/utils/VRFunction.h"
#include "core/scene/VRScene.h"
#include "core/scene/VRSceneManager.h"

OSG_BEGIN_NAMESPACE;

VRImport::VRImport() {
    progress = VRProgress::create();
}

VRImport* VRImport::get() {
    static VRImport* s = new VRImport();
    return s;
}

void VRImport::fixEmptyNames(NodeMTRecPtr o, map<string, bool>& m, string parentName, int iChild) {
    if (!OSG::getName(o)) {
        stringstream ss; ss << parentName << "_" << iChild;
        OSG::setName(o, ss.str());
    }

    // make unique
    string name = getName(o);
    string orig = name;
    for (int i=0; m.count(name); i++) {
        stringstream ss; ss << orig << i;
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

VRTransformPtr VRImport::Cache::retrieve(VRObjectPtr parent) {
    if (copy == 0) copy = static_pointer_cast<VRTransform>(root->duplicate()); // keep a copy, TODO: try to change the namespace of the copy, maybe helpful
    else root = static_pointer_cast<VRTransform>(copy->duplicate());
    if (parent) parent->addChild(root);
    return root;
}

void VRImport::osgLoad(string path, VRObjectPtr res) {
    cout << "OSG Load " << path << endl;
    res->setName(path);
    NodeMTRecPtr n = SceneFileHandler::the()->read(path.c_str());
    if (n == 0) return;
    map<string, bool> m;
    fixEmptyNames(n,m);
    res->addChild( OSGConstruct(n, res, path, path) );
}

int fileSize(string path) {
    ifstream in(path, ios::binary | ios::ate);
    int L = in.tellg();
    in.close();
    return L;
}

VRTransformPtr VRImport::load(string path, VRObjectPtr parent, bool reload, string preset, bool thread) {
    cout << "VRImport::load " << path << " " << preset << endl;
    if (ihr_flag) if (fileSize(path) > 3e7) return 0;
    setlocale(LC_ALL, "C");

    // check cache
    cout << "RELOAD " << reload << endl;
    reload = reload ? true : (cache.count(path) == 0);
    cout << "RELOAD " << reload << endl;
    if (!reload) {
        auto res = cache[path].retrieve(parent);
        cout << "load " << path << " : " << res << " from cache!\n";
        return res;
    }

    // check file path
    if (!boost::filesystem::exists(path)) { cout << "VRImport::load " << path << " not found!" << endl; return 0; }

    VRTransformPtr res = VRTransform::create("proxy");
    LoadJob* job = new LoadJob(path, preset, res, progress);
    if (!thread) job->load(VRThreadWeakPtr());
    else {
        job->loadCb = VRFunction< VRThreadWeakPtr >::create( "geo load", boost::bind(&LoadJob::load, job, _1) );
        int t = VRSceneManager::getCurrent()->initThread(job->loadCb.get(), "geo load thread", false, 1);
    }

    fillCache(path, res);
    return cache[path].retrieve(parent);
}

VRImport::LoadJob::LoadJob(string p, string pr, VRTransformPtr r, VRProgressPtr pg) {
    path = p;
    res = r;
    progress = pg;
    preset = pr;
}

void VRImport::LoadJob::load(VRThreadWeakPtr thread) {
    VRThreadPtr t = thread.lock();

    if (t) t->syncFromMain();

    auto loadSwitch = [&]() {
        auto bpath = boost::filesystem::path(path);
        string ext = bpath.extension().string();
        cout << "load " << path << " ext: " << ext << " preset: " << preset << "\n";
        if (ext == ".e57") { loadE57(path, res); return; }
        if (ext == ".ply") { loadPly(path, res); return; }
        if (ext == ".stp") { VRSTEP step; step.load(path, res); return; }
        if (ext == ".wrl" && preset == "SOLIDWORKS-VRML2") { VRFactory f; f.loadVRML(path, progress, res); return; }
        if (preset == "OSG" || preset == "COLLADA") osgLoad(path, res);
        if (preset == "COLLADA") loadCollada(path, res);
    };

    loadSwitch();
    if (t) t->syncToMain();
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

    if (name[0] == 'F' && name[1] == 'T') {
        string g = name; g.erase(0,2);
        if (g.find('.') != string::npos) g.erase(g.find('.'));
        if (g.find('_') != string::npos) g.erase(g.find('_'));

        tmp_gr = VRGroup::create(g);
        tmp_gr->setActive(true);
        tmp_gr->setGroup(g);
        tmp = tmp_gr;

        if (t_name == "Transform") {
            tmp_e = VRTransform::create(g);
            tmp_e->setMatrix(dynamic_cast<Transform *>(n->getCore())->getMatrix());
            tmp = tmp_e;
            tmp->addChild(tmp_gr);
        }

        for (uint i=0;i<n->getNChildren();i++)
            tmp_gr->addChild(OSGConstruct(n->getChild(i), parent, name, geoTransName));

        return tmp;
    }

    else if (t_name == "Group") {//OpenSG Group
        tmp = VRObject::create(name);
        tmp->setCore(core, "Object");
        tmp->addAttachment("collada_name", name);
    }

    else if (t_name == "ComponentTransform") {
        if (tmp == 0) {
            tmp_e = VRTransform::create(name);
            tmp_e->setMatrix(dynamic_cast<ComponentTransform *>(n->getCore())->getMatrix());
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
            tmp_e->setMatrix(dynamic_cast<Transform *>(n->getCore())->getMatrix());
            tmp = tmp_e;
            tmp->addAttachment("collada_name", name);
        }
    }

    else if (t_name == "MaterialGroup") {
        tmp_m = VRMaterial::create(name);
        tmp = tmp_m;
        tmp->setCore(core, "Material");
    }

    else if (t_name == "Geometry") {
        tmp_g = VRGeometry::create(name);
        if (geoTrans) {
            tmp_g->addAttachment("collada_name", geoTransName);
            tmp_g->setMatrix(dynamic_cast<Transform *>(geoTrans)->getMatrix());
            geoTrans = 0;
            geoTransName = "";
        }

        VRGeometry::Reference ref;
        ref.type = VRGeometry::FILE;
        ref.parameter = currentFile + " " + name;
        tmp_g->setMesh(dynamic_cast<Geometry *>(n->getCore()), ref, true);
        tmp = tmp_g;
    }

    else {
        tmp = VRObject::create(name);
        tmp->setCore(core, t_name);
    }

    for (uint i=0;i<n->getNChildren();i++)
        tmp->addChild(OSGConstruct(n->getChild(i), tmp, name, currentFile, geoTrans, geoTransName));

    return tmp;
}

void VRImport::fillCache(string path, VRTransformPtr obj) {
    if (cache.count(path) == 0) cache[path] = Cache();
    cache[path].root = static_pointer_cast<VRTransform>(obj);
    for (auto o : cache[path].root->getChildren(true)) cache[path].objects[o->getName()] = o;
    cache[path].copy = 0; // TODO
}

VRGeometryPtr VRImport::loadGeometry(string file, string object) {
    if (cache.count(file) == 0) load(file);

    if (cache.count(file) == 0) {
        cout << "VRImport::loadGeometry - Warning: " << file << " not in cache" << endl;
        return 0;
    }

    if (cache[file].objects.count(object) == 0) {
        cout << "VRImport::loadGeometry - Warning: " << file << " in cache but has no object " << object << endl;
        cout << cache[file].root->getName() << endl;
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

VRImport::Cache::Cache() {;}
VRImport::Cache::Cache(VRTransformPtr root) {
    this->root = root;
    for (auto c : root->getChildren(true)) objects[getName(c->getNode())] = c;
}

VRProgressPtr VRImport::getProgressObject() { return progress; }

void VRImport::ingoreHeavyRessources() { ihr_flag = true; }


OSG_END_NAMESPACE;



