#include "VRImport.h"
#include "VRCOLLADA.h"
#include "VRPLY.h"

#include <OpenSG/OSGSceneFileHandler.h>
#include <OpenSG/OSGNameAttachment.h>
#include <OpenSG/OSGComponentTransform.h>

#include "core/objects/VRTransform.h"
#include "core/objects/VRGroup.h"
#include "core/objects/object/VRObject.h"
#include "core/objects/object/VRObjectT.h"
#include "core/objects/geometry/VRGeometry.h"

OSG_BEGIN_NAMESPACE;

VRImport::VRImport() {;}

VRImport* VRImport::get() {
    static VRImport* s = new VRImport();
    return s;
}

void VRImport::fixEmptyNames(NodeRecPtr o, map<string, bool>& m, string parentName, int iChild) {
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

VRTransform* VRImport::prependTransform(VRObject* o, string path) {
    if (o->getChildrenCount() == 1)
        if (o->getChild(0)->getType() == "Transform")
            return (VRTransform*)o->getChild(0);

    boost::filesystem::path p(path);
    auto trans = new VRTransform( p.filename().string() );
    trans->addChild(o);
    return trans;
}

VRTransform* VRImport::Cache::retrieve() {
    if (copy == 0) copy = (VRTransform*)root->duplicate(); // keep a copy, TODO: try to change the namespace of the copy, maybe helpful
    else root = (VRTransform*)copy->duplicate();
    return root;
}

VRTransform* VRImport::load(string path, VRObject* parent, bool reload, string preset) {           cout << "VRImport::load " << path << endl;
    reload = reload? true : (cache.count(path) == 0);
    if (!reload) return cache[path].retrieve();
    if (path.size() < 4) return 0;

    setlocale(LC_ALL, "C");

    if (preset == "PLY") {
        VRGeometry* geo = loadPly(path);
        return geo; // TODO: use cache!
    }

    if (preset == "OSG" || preset == "COLLADA") { // TODO: using OSG importer for collada geometries
        NodeRecPtr n = 0;
        n = SceneFileHandler::the()->read(path.c_str());
        if (n == 0) return 0;

        map<string, bool> m;
        fixEmptyNames(n,m);
        OSGConstruct(n, parent, path, path);
    }

    if (preset == "COLLADA") {
        cout << "load collada - root " << cache[path].root->getName() << endl;
        loadCollada(path, cache[path].root); // TODO: use cache!
    }

    cache[path].root = prependTransform(cache[path].root, path);
    if (parent) parent->addChild(cache[path].root);
    return cache[path].retrieve();
}

VRObject* VRImport::OSGConstruct(NodeRecPtr n, VRObject* parent, string name, string currentFile, NodeCore* geoTrans, string geoTransName) {
    if (n == 0) return 0; // TODO add an osg wrap method for each object?

    VRObject* tmp = 0;
    VRGeometry* tmp_g;
    VRTransform* tmp_e;
    VRGroup* tmp_gr;

    NodeCoreRecPtr core = n->getCore();
    string t_name = core->getTypeName();


    if (getName(n)) name = getName(n);
    else name = "Unnamed";
    if (name == "") name = "NAN";

    if (name[0] == 'F' && name[1] == 'T') {
        string g = name; g.erase(0,2);
        if (g.find('.') != string::npos) g.erase(g.find('.'));
        if (g.find('_') != string::npos) g.erase(g.find('_'));

        tmp_gr = new VRGroup(g);
        tmp_gr->setActive(true);
        tmp_gr->setGroup(g);
        tmp = tmp_gr;

        if (t_name == "Transform") {
            tmp_e = new VRTransform(g);
            tmp_e->setMatrix(dynamic_cast<Transform *>(n->getCore())->getMatrix());
            tmp = tmp_e;
            tmp->addChild(tmp_gr);
        }

        for (uint i=0;i<n->getNChildren();i++)
            tmp_gr->addChild(OSGConstruct(n->getChild(i), parent, name, geoTransName));

        return tmp;
    }

    else if (t_name == "Group") {//OpenSG Group
        tmp = new VRObject(name);
        tmp->setCore(core, "Object");
        tmp->addAttachment("collada_name", name);
    }

    else if (t_name == "ComponentTransform") {
        if (tmp == 0) {
            tmp_e = new VRTransform(name);
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
            tmp_e = new VRTransform(name);
            tmp_e->setMatrix(dynamic_cast<Transform *>(n->getCore())->getMatrix());
            tmp = tmp_e;
            tmp->addAttachment("collada_name", name);
        }
    }

    else if (t_name == "MaterialGroup") {
        cout << "Warning: unsupported MaterialGroup\n";
        tmp = new VRObject(name);
        tmp->setCore(core, t_name);
    }

    else if (t_name == "Geometry") {
        tmp_g = new VRGeometry(name);
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
        tmp = new VRObject(name);
        tmp->setCore(core, t_name);
    }

    for (uint i=0;i<n->getNChildren();i++)
        tmp->addChild(OSGConstruct(n->getChild(i), tmp, name, currentFile, geoTrans, geoTransName));

    if (cache.count(currentFile) == 0) cache[currentFile] = Cache();
    cache[currentFile].objects[name] = tmp;
    cache[currentFile].root = (VRTransform*)tmp; // TODO
    cache[currentFile].copy = 0; // TODO
    return tmp;
}

VRGeometry* VRImport::loadGeometry(string file, string object) {
    if (cache.count(file) == 0) load(file);

    if (cache.count(file) == 0) {
        cout << "VRSceneLoader::loadGeometry - Warning: " << file << " not in cache" << endl;
        return 0;
    }

    if (cache[file].objects.count(object) == 0) {
        cout << "VRSceneLoader::loadGeometry - Warning: " << file << " in cache but has no object" << object << endl;
        for (auto o : cache[file].objects) cout << "cache " << o.first << endl;
        return 0;
    }

    VRObject* o = cache[file].objects[object];
    if (o->getType() != "Geometry") {
        cout << "VRSceneLoader::loadGeometry - Warning: " << file << " is cached but object " << object << " has wrong type: " << o->getType() << endl;
        return 0;
    }

    return (VRGeometry*)o;
}

VRImport::Cache::Cache() {;}
VRImport::Cache::Cache(VRTransform* root) {
    this->root = root;
    for (auto c : root->getChildren(true)) objects[getName(c->getNode())] = c;
}

OSG_END_NAMESPACE;
