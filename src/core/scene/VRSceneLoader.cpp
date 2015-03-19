#include "VRSceneLoader.h"
#include "VRSceneManager.h"
#include "core/objects/VRGroup.h"
#include "core/objects/VRLight.h"
#include "core/objects/VRLightBeacon.h"
#include "core/objects/geometry/VRGeometry.h"
//#include "addons/Engineering/CSG/CSGGeometry.h"

#include "VRScene.h"
#include "core/utils/VRTimer.h"
#include "core/navigation/VRNavigator.h"
#include "core/objects/VRLod.h"
#include "addons/construction/building/VROpening.h"

#include <libxml++/libxml++.h>
#include <libxml++/nodes/element.h>
#include <GL/glut.h>

#include <OpenSG/OSGSimpleMaterial.h>
#include <OpenSG/OSGPointLight.h>
#include <OpenSG/OSGGeoProperties.h>
#include <OpenSG/OSGComponentTransform.h>

#include <stdio.h>
#include <fstream>

OSG_BEGIN_NAMESPACE;
using namespace std;

VRScene* VRSceneLoader_current_scene = 0;

void fixEmptyNames(NodeRecPtr o, string parentName = "NAN", int iChild = 0) {
    string name;
    if (OSG::getName(o)) {
        name = OSG::getName(o);
        //std::replace( name.begin(), name.end(), ' ', '_'); // senseless, the osg importer allready missed the blanks! :(
        //OSG::setName(o, name.c_str());
    } else {
        stringstream ss; ss << parentName << "_" << iChild;
        OSG::setName(o, ss.str());
    }

    for (uint i=0; i<o->getNChildren(); i++) {
        fixEmptyNames(o->getChild(i), name, i);
    }
}

void fillNodeMap(map<string, NodeRecPtr>& m, NodeRecPtr n) {
    string name = getName(n);

    string orig = name;
    int i=0; // fix, when the names are not unique
    while (m.count(name)) {
        stringstream ss; ss << orig << i;
        name = ss.str();
        i++;
    }

    setName(n, name.c_str());
    m[name] = n;
    for (uint i=0; i<n->getNChildren(); i++)
        fillNodeMap(m, n->getChild(i));
}

NodeRecPtr loadPly(string filename) {
    ifstream file(filename.c_str());
    string s;
    for(int i=0; i<16; i++) getline(file, s); // jump to data

    GeoUInt8PropertyRecPtr      Type = GeoUInt8Property::create();
    GeoUInt32PropertyRecPtr     Length = GeoUInt32Property::create();
    GeoPnt3fPropertyRecPtr      Pos = GeoPnt3fProperty::create();
    GeoVec3fPropertyRecPtr      Norms = GeoVec3fProperty::create();
    GeoVec3fPropertyRecPtr      Cols = GeoVec3fProperty::create();
    GeoUInt32PropertyRecPtr     Indices = GeoUInt32Property::create();
    SimpleMaterialRecPtr        Mat = SimpleMaterial::create();
    GeoVec2fPropertyRecPtr      Tex = GeoVec2fProperty::create();

    Mat->setLit(false);
    Mat->setDiffuse(Color3f(0.8,0.8,0.6));
    Mat->setAmbient(Color3f(0.4, 0.4, 0.2));
    Mat->setSpecular(Color3f(0.1, 0.1, 0.1));

    int N = 0;
    string line;
    while (std::getline(file, line)) {
        istringstream iss(line);
        Vec3f p, n;
        Vec3i c;
        if (!(iss >> p[0] >> p[1] >> p[2] >> n[0] >> n[1] >> n[2] >> c[0] >> c[1] >> c[2])) {
            cout << "\nBREAK PLY IMPORT AT " << N << endl;
            break;
        } // error

        Pos->addValue(p);
        Norms->addValue(n);
        Cols->addValue(Vec3f(c[0]/255., c[1]/255., c[2]/255.));
        Indices->addValue(N);
        N++;
    }

    Type->addValue(GL_POINTS);
    Length->addValue(N);

    GeometryRecPtr geo = Geometry::create();
    geo->setTypes(Type);
    geo->setLengths(Length);
    geo->setIndices(Indices);
    geo->setPositions(Pos);
    geo->setNormals(Norms);
    geo->setColors(Cols);
    geo->setMaterial(Mat);

    file.close();
    return makeNodeFor(geo);
}

void VRSceneLoader::load(string filename) {
    if (ihr_flag) { // check
		ifstream in(filename, ios::binary | ios::ate);
		int L = in.tellg();
		in.close();
        cout << "File " << filename << " has size " << L << endl;
        if (L > 30000000) return;
    }

    if (filename.size() < 4) return;
    string extension = filename.substr(filename.size()-4, filename.size()-1);

    VRTimer timer;
    timer.start("scenefilehandler");

    NodeRecPtr n;
    if (extension == ".ply") n = loadPly(filename);
    else {
        cout << "read " << filename << flush;
        setlocale(LC_ALL, "C");
        n = SceneFileHandler::the()->read(filename.c_str());
        cout << ", done " << endl;
    }

    timer.stop("scenefilehandler");
    if (n == 0) return;

    cached_files[filename] = cache();
    cached_files[filename].root = n;
    timer.start("fixnames");
    fixEmptyNames(n);
    timer.start("fixnames");

    timer.start("fillnodemap");
    fillNodeMap(cached_files[filename].nodes, n);
    timer.stop("fillnodemap");
    //timer.print();
}

VRObject* VRSceneLoader::parseOSGTree(NodeRecPtr n, VRObject* parent, string name, string currentFile, NodeCore* geoTrans) { // TODO add a wrap method for each object?
    if (n == 0) return 0;

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
            tmp_gr->addChild(parseOSGTree(n->getChild(i), parent, name));

        return tmp;
    }

    else if (t_name == "Group") {//OpenSG Group
        tmp = new VRObject(name);
        tmp->setCore(core, "Object");
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
                tmp = parent;
            }
        }

        if (tmp == 0) {
            tmp_e = new VRTransform(name);
            tmp_e->setMatrix(dynamic_cast<Transform *>(n->getCore())->getMatrix());
            tmp = tmp_e;
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
            tmp_g->setMatrix(dynamic_cast<Transform *>(geoTrans)->getMatrix());
            geoTrans = 0;
        }

        VRGeometry::Reference ref;
        ref.type = VRGeometry::FILE;
        ref.parameter = currentFile + " " + name;
        tmp_g->setMesh(dynamic_cast<Geometry *>(n->getCore()), ref, true);
        tmp = tmp_g;
    }

    else {
        //tmp = new VRObject(name + " _BAD PARSE: " + t_name + "_ ");
        tmp = new VRObject(name);
        tmp->setCore(core, t_name);
    }

    for (uint i=0;i<n->getNChildren();i++)
        tmp->addChild(parseOSGTree(n->getChild(i), tmp, name, currentFile, geoTrans));

    return tmp;
}

void VRSceneLoader::optimizeGraph(VRObject* obj) { //TODO
    VRObject* p = obj->getParent();
    if (obj->getType() == "Geometry" && p->getType() == "Transform" && p->getChildrenCount() == 1) {
        obj->switchParent(p->getParent());
        obj->setName(p->getName());
        //obj->setMatrix(p->getMatrix()); //TODO: cast
        p->hide();
    }

    for (uint i=0;i<obj->getChildrenCount();i++)
        optimizeGraph(obj->getChild(i));
}

void VRSceneLoader::fixLocalLightsImport(VRObject* anchor) {
    cout << "\nImport lights" << flush;

    //Create a list of all lights
    vector<VRObject*> lights = anchor->filterByType("PointLight");

    //get lightbeacon with similar name (Lamp.001 -> Lamp_001)
    //set light node as parent of lightbeacon
    for (uint i=0;i<lights.size();i++) {

        //get beacon
        string b = lights[i]->getName();
        replace(b.begin(), b.end(), '.', '_');
        b += "Beacon";
        VRObject* beacon = anchor->find(b);

        if (beacon == 0) {
            cout << "\nWarning: light beacon " << b << " not found!" << flush;
            continue;
        }

        while (lights[i]->getChildrenCount() != 0)
            lights[i]->getChild(0)->switchParent(lights[i]->getParent());

        //lights[i]->switchParent(beacon->getParent());
        beacon->switchParent(lights[i]);

        PointLightRecPtr osglight = dynamic_cast<PointLight*>(lights[i]->getNode()->getCore());
        osglight->setBeacon(beacon->getNode());

        scene->addDSLight(osglight, "point");
    }
}

VRSceneLoader::VRSceneLoader() { cout << "\nInit VRSceneLoader\n"; }

VRSceneLoader::~VRSceneLoader() {
    ;
}

VRSceneLoader* VRSceneLoader::get() {
    static VRSceneLoader* s = new VRSceneLoader();
    return s;
}

// get only the object for a single geometry
GeometryRecPtr VRSceneLoader::loadGeometry(string file, string object) {
    VRScene* scene = VRSceneLoader_current_scene;
    if (scene == 0) scene = VRSceneManager::getCurrent();
    if (scene == 0) return 0;

    if (cached_files.count(file) == 0) load(file);
    if (cached_files.count(file) == 0) {
        cout << "\n VRSceneLoader::loadGeometry - Warning: " << file << " not in cache" << flush;
        return 0;
    }
    if (cached_files[file].nodes.count(object) == 0) {
        cout << "\n VRSceneLoader::loadGeometry - Warning: " << file << " in cache but has no object" << object << flush;
        return 0;
    }

    NodeRecPtr n = cached_files[file].nodes[object];
    NodeCore* c = n->getCore();
    string type = c->getTypeName();

    if (type == "Geometry") return dynamic_cast<Geometry *>(n->getCore());
    cout << "\n VRSceneLoader::loadGeometry - Warning: " << file << " is cached but object " << object << " has wrong type: " << type << flush;
    return 0;
}

VRTransform* VRSceneLoader::load3DContent(string filepath, VRObject* parent, bool reload) {
    VRScene* scene = VRSceneLoader_current_scene;
    if (scene == 0) scene = VRSceneManager::getCurrent();
    if (scene == 0) return 0;

    cout << "load3DContent " << filepath << endl;

    VRObject* root;
    if(cached_files.count(filepath) == 0 || reload) load(filepath);
    NodeRecPtr osg = cached_files[filepath].root;
    root = parseOSGTree(osg, parent, filepath, filepath);
    if (root == 0) return 0;

    //optimizeGraph(root);
    string filename = filepath;
    uint i = filename.find_last_of("\\/");
    if (i != string::npos) filename.erase(0, i + 1);
    i= filename.rfind('.');
    if (i != string::npos) filename.erase(i);


    VRTransform* trans = 0;
    if (root->getChildrenCount() == 1)
        if (root->getChild(0)->getType() == "Transform")
            trans = (VRTransform*)root->getChild(0);

    if (trans == 0) {
        trans = new VRTransform(filename);
        trans->addChild(root);
    }

    if (parent) parent->addChild(trans);

    fixLocalLightsImport(root);

    return trans;
}


void VRSceneLoader_saveObject(VRObject* p, xmlpp::Element* e) {
    if (e == 0) return;
    p->save(e);
    for (uint i=0; i<p->getChildrenCount(); i++) {
        VRObject* c = p->getChild(i);
        if (c->hasAttachment("dynamicaly_generated")) continue; // generated objects are not be saved
        if (c->hasAttachment("global")) continue; // global objects are not be saved
        //xmlpp::Element* ce = e->add_child(c->getName());
        xmlpp::Element* ce = e->add_child("Object");
        VRSceneLoader_saveObject(c, ce);
    }
}

void VRSceneLoader::saveScene(string file, xmlpp::Element* guiN) {
    cout << " save " << file << endl;
    VRScene* scene = VRSceneManager::getCurrent();
    if (scene == 0) return;

    xmlpp::Document doc;
    xmlpp::Element* sceneN = doc.create_root_node("Scene", "", "VRF"); //name, ns_uri, ns_prefix
    xmlpp::Element* objectsN = sceneN->add_child("Objects");
    if (guiN) sceneN->import_node(guiN, true);

    // save scenegraph
    scene->setPath(file);
    VRObject* root = scene->getRoot();
    xmlpp::Element* rootN = objectsN->add_child("Object");
    VRSceneLoader_saveObject(root, rootN);
    scene->save(sceneN);
    doc.write_to_file_formatted(file);
}

VRObject* VRSceneLoader_createFromElement(VRScene* scene, xmlpp::Element* e) {
    string type = e->get_attribute("type")->get_value();
    string base_name = e->get_attribute("base_name")->get_value();
    //string name = e->get_name();


    if (type == "Transform") return new VRTransform(base_name);
    if (type == "Geometry") return new VRGeometry(base_name);
    //if (type == "CSGGeometry") return new CSGGeometry(base_name);
    if (type == "Camera") return scene->addCamera(base_name);
    if (type == "LightBeacon") return new VRLightBeacon(base_name);
    if (type == "Light") return scene->addLight(base_name);
    if (type == "Group") return new VRGroup(base_name);
    if (type == "Lod") return new VRLod(base_name);

    if (type != "Object") cout << "\nERROR in VRSceneLoader_createFromElement: " << type << " is not handled!\n";
    return new VRObject(base_name);
}

void VRSceneLoader_loadObject(VRScene* scene, VRObject* p, xmlpp::Element* e) {
    if (e == 0) return;
    xmlpp::Node::NodeList nl = e->get_children();
    xmlpp::Node::NodeList::iterator itr;
    for (itr = nl.begin(); itr != nl.end(); itr++) {
        xmlpp::Node* n = *itr;

        xmlpp::Element* el = dynamic_cast<xmlpp::Element*>(n);
        if (!el) continue;

        VRObject* c = VRSceneLoader_createFromElement(scene, el);

        p->addChild(c);
        c->load(el);
        VRSceneLoader_loadObject(scene, c, el);
    }
}

xmlpp::Element* VRSceneLoader_getElementChild_(xmlpp::Element* e, string name) {
    if (e == 0) return 0;
    xmlpp::Node::NodeList nl = e->get_children();
    xmlpp::Node::NodeList::iterator itr;
    for (itr = nl.begin(); itr != nl.end(); itr++) {
        xmlpp::Node* n = *itr;

        xmlpp::Element* el = dynamic_cast<xmlpp::Element*>(n);
        if (!el) continue;

        if (el->get_name() == name) return el;
    }
    return 0;
}

xmlpp::Element* VRSceneLoader_getElementChild_(xmlpp::Element* e, int i) {
    if (e == 0) return 0;
    xmlpp::Node::NodeList nl = e->get_children();
    xmlpp::Node::NodeList::iterator itr;
    int j = 0;
    for (itr = nl.begin(); itr != nl.end(); itr++) {
        xmlpp::Node* n = *itr;
        xmlpp::Element* el = dynamic_cast<xmlpp::Element*>(n);
        if (!el) continue;

        if (i == j) return el;
        j++;
    }

    return 0;
}

void VRSceneLoader::loadScene(string path) {
    xmlpp::DomParser parser;
    parser.set_validate(false);
    parser.parse_file(path.c_str());

    xmlpp::Node* n = parser.get_document()->get_root_node();
    xmlpp::Element* sceneN = dynamic_cast<xmlpp::Element*>(n);

    // load scenegraph
    xmlpp::Element* objectsN = VRSceneLoader_getElementChild_(sceneN, "Objects");
    xmlpp::Element* root = VRSceneLoader_getElementChild_(objectsN, 0);
    VRScene* scene = new VRScene();
    scene->setPath(path);
    VRSceneManager::get()->setWorkdir(scene->getWorkdir());
    scene->setName(scene->getFileName());
    VRSceneLoader_current_scene = scene;

    VRTimer timer;
    timer.start("total_time");
    VRSceneLoader_loadObject(scene, scene->getRoot(), root);
    timer.stop("total_time");

    VRSceneManager::get()->addScene(scene);

    scene->load(sceneN);
    //timer.print();

    ihr_flag = false;
}

void VRSceneLoader::ingoreHeavyRessources() { ihr_flag = true; }

OSG_END_NAMESPACE;
