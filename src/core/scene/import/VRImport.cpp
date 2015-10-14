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

VRTransformPtr VRImport::prependTransform(VRObjectPtr o, string path) {
    if (o->getChildrenCount() == 1)
        if (o->getChild(0)->getType() == "Transform")
            return static_pointer_cast<VRTransform>(o->getChild(0));

    boost::filesystem::path p(path);
    auto trans = VRTransform::create( p.filename().string() );
    trans->addChild(o);
    return trans;
}

VRTransformPtr VRImport::Cache::retrieve() {
    if (copy == 0) copy = static_pointer_cast<VRTransform>(root->duplicate()); // keep a copy, TODO: try to change the namespace of the copy, maybe helpful
    else root = static_pointer_cast<VRTransform>(copy->duplicate());
    return root;
}

VRTransformPtr VRImport::load(string path, VRObjectPtr parent, bool reload, string preset) {           cout << "VRImport::load " << path << endl;
    reload = reload? true : (cache.count(path) == 0);
    if (!reload) {
        auto res = cache[path].retrieve();
        if (parent) parent->addChild(res);
        cout << "load " << path << " : " << res << " from cache!\n";
        return res;
    }

    if (path.size() < 4) return 0;

    setlocale(LC_ALL, "C");

    if (preset == "PLY") {
        VRGeometryPtr geo = loadPly(path);
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

    auto res = cache[path].retrieve();
    cout << "load " << path << " : " << res << endl;
    return res;
}

VRObjectPtr VRImport::OSGConstruct(NodeRecPtr n, VRObjectPtr parent, string name, string currentFile, NodeCore* geoTrans, string geoTransName) {
    if (n == 0) return 0; // TODO add an osg wrap method for each object?

    VRObjectPtr tmp = 0;
    VRGeometryPtr tmp_g;
    VRTransformPtr tmp_e;
    VRGroupPtr tmp_gr;

    NodeCoreRecPtr core = n->getCore();
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
        cout << "Warning: unsupported MaterialGroup\n";
        tmp = VRObject::create(name);
        tmp->setCore(core, t_name);
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

    if (cache.count(currentFile) == 0) cache[currentFile] = Cache();
    cache[currentFile].objects[name] = tmp;
    cache[currentFile].root = static_pointer_cast<VRTransform>(tmp); // TODO
    cache[currentFile].copy = 0; // TODO
    return tmp;
}

VRGeometryPtr VRImport::loadGeometry(string file, string object) {
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

    VRObjectPtr o = cache[file].objects[object];
    if (o->getType() != "Geometry") {
        cout << "VRSceneLoader::loadGeometry - Warning: " << file << " is cached but object " << object << " has wrong type: " << o->getType() << endl;
        return 0;
    }

    return static_pointer_cast<VRGeometry>(o);
}

VRImport::Cache::Cache() {;}
VRImport::Cache::Cache(VRTransformPtr root) {
    this->root = root;
    for (auto c : root->getChildren(true)) objects[getName(c->getNode())] = c;
}

OSG_END_NAMESPACE;
