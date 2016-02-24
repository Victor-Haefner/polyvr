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

#include <OpenSG/OSGGeometry.h>
#include <OpenSG/OSGSimpleMaterial.h>
#include <OpenSG/OSGPointLight.h>
#include <OpenSG/OSGGeoProperties.h>
#include <OpenSG/OSGComponentTransform.h>
#include <OpenSG/OSGNameAttachment.h>

#include <stdio.h>
#include <fstream>
#include <unistd.h>
#include <boost/filesystem.hpp>

#include "import/VRImport.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

VRSceneWeakPtr VRSceneLoader_current_scene;

void VRSceneLoader::optimizeGraph(VRObjectPtr obj) { //TODO
    VRObjectPtr p = obj->getParent();
    if (obj->getType() == "Geometry" && p->getType() == "Transform" && p->getChildrenCount() == 1) {
        obj->switchParent(p->getParent());
        obj->setName(p->getName());
        //obj->setMatrix(p->getMatrix()); //TODO: cast
        p->hide();
    }

    for (uint i=0;i<obj->getChildrenCount();i++)
        optimizeGraph(obj->getChild(i));
}

VRSceneLoader::VRSceneLoader() { cout << "\nInit VRSceneLoader\n"; }
VRSceneLoader::~VRSceneLoader() { ; }

VRSceneLoader* VRSceneLoader::get() {
    static VRSceneLoader* s = new VRSceneLoader();
    return s;
}

int VRSceneLoader::fileSize(string path) {
    ifstream in(path, ios::binary | ios::ate);
    int L = in.tellg();
    in.close();
    return L;
}

VRTransformPtr VRSceneLoader::load3DContent(string filepath, VRObjectPtr parent, bool reload) {
    if (ihr_flag) if (fileSize(filepath) > 3e7) return 0;
    return VRImport::get()->load(filepath, parent, reload);
}

void VRSceneLoader_saveObject(VRObjectPtr p, xmlpp::Element* e) {
    if (e == 0) return;
    p->saveContent(e);
    for (uint i=0; i<p->getChildrenCount(); i++) {
        VRObjectPtr c = p->getChild(i);
        if (c->getPersistency() == 0) continue; // generated objects are not be saved
        //xmlpp::Element* ce = e->add_child(c->getName());
        xmlpp::Element* ce = e->add_child("Object");
        VRSceneLoader_saveObject(c, ce);
    }
}

void VRSceneLoader::saveScene(string file, xmlpp::Element* guiN) {
    if (boost::filesystem::exists(file))
        file = boost::filesystem::canonical(file).string();
    cout << " save " << file << endl;
    auto scene = VRSceneManager::getCurrent();
    if (scene == 0) return;

    xmlpp::Document doc;
    xmlpp::Element* sceneN = doc.create_root_node("Scene", "", "VRF"); //name, ns_uri, ns_prefix
    xmlpp::Element* objectsN = sceneN->add_child("Objects");
    if (guiN) sceneN->import_node(guiN, true);

    // save scenegraph
    scene->setPath(file);
    VRObjectPtr root = scene->getRoot();
    //xmlpp::Element* rootN = objectsN->add_child("Object");
    //VRSceneLoader_saveObject(root, rootN);
    root->saveUnder(objectsN);
    scene->save(sceneN);
    doc.write_to_file_formatted(file);
}

VRObjectPtr VRSceneLoader_createFromElement(VRScenePtr scene, xmlpp::Element* e) {
    string type = e->get_attribute("type")->get_value();
    string base_name = e->get_attribute("base_name")->get_value();
    //string name = e->get_name();


    if (type == "Transform") return VRTransform::create(base_name);
    if (type == "Geometry") return VRGeometry::create(base_name);
    //if (type == "CSGGeometry") return new CSGGeometry(base_name);
    if (type == "Camera") return scene->addCamera(base_name);
    if (type == "LightBeacon") return VRLightBeacon::create(base_name);
    if (type == "Light") return scene->addLight(base_name);
    if (type == "Group") return VRGroup::create(base_name);
    if (type == "Lod") return VRLod::create(base_name);

    if (type != "Object") cout << "\nERROR in VRSceneLoader_createFromElement: " << type << " is not handled!\n";
    return VRObject::create(base_name);
}

void VRSceneLoader_loadObject(VRScenePtr scene, VRObjectPtr p, xmlpp::Element* e) {
    if (e == 0) return;
    xmlpp::Node::NodeList nl = e->get_children();
    xmlpp::Node::NodeList::iterator itr;
    for (itr = nl.begin(); itr != nl.end(); itr++) {
        xmlpp::Node* n = *itr;

        xmlpp::Element* el = dynamic_cast<xmlpp::Element*>(n);
        if (!el) continue;

        VRObjectPtr c = VRSceneLoader_createFromElement(scene, el);

        p->addChild(c);
        c->loadContent(el);
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
    VRSceneManager::get()->closeScene();
    auto scene = VRScenePtr( new VRScene() );
    scene->setPath(path);
    VRSceneManager::get()->setWorkdir(scene->getWorkdir());
    scene->setName(scene->getFileName());
    VRSceneLoader_current_scene = scene;

    VRTimer timer;
    timer.start("total_time");
    VRSceneLoader_loadObject(scene, scene->getRoot(), root);
    timer.stop("total_time");

    VRSceneManager::get()->setScene(scene);

    scene->load(sceneN);
    //timer.print();

    ihr_flag = false;
}

void VRSceneLoader::ingoreHeavyRessources() { ihr_flag = true; }


OSG_END_NAMESPACE;
