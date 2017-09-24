#include "VRSceneLoader.h"
#include "VRSceneManager.h"
#include "core/objects/VRGroup.h"
#include "core/objects/VRCamera.h"
#include "core/objects/VRLight.h"
#include "core/objects/VRLightBeacon.h"
#include "core/objects/geometry/VRGeometry.h"

#include "core/tools/VRWaypoint.h"
#include "core/tools/VRGeoPrimitive.h"
#include "core/tools/VRKinematictool.h"
//#include "addons/Engineering/CSG/CSGGeometry.h"

#include "VRScene.h"
#include "core/utils/VRTimer.h"
#include "core/utils/VRStorage_template.h"
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

void regObjectStorageTypes() {
    VRStorage::regStorageType<VRObject>("Object");
    VRStorage::regStorageType<VRTransform>("Transform");
    VRStorage::regStorageType<VRGeometry>("Geometry");
    VRStorage::regStorageType<VRCamera>("Camera");
    VRStorage::regStorageType<VRLight>("Light");
    VRStorage::regStorageType<VRLightBeacon>("LightBeacon");
    VRStorage::regStorageType<VRGroup>("Group");
    VRStorage::regStorageType<VRLod>("Lod");
    VRStorage::regStorageType<VRWaypoint>("Waypoint");
    VRStorage::regStorageType<VRGeoPrimitive>("GeoPrimitive");
    VRStorage::regStorageType<VRJointTool>("JointTool");
}

VRSceneLoader::VRSceneLoader() { cout << "\nInit VRSceneLoader\n"; regObjectStorageTypes(); }
VRSceneLoader::~VRSceneLoader() { ; }

VRSceneLoader* VRSceneLoader::get() {
    static VRSceneLoader* s = new VRSceneLoader();
    return s;
}

void VRSceneLoader::saveScene(string file, xmlpp::Element* guiN) {
    if (boost::filesystem::exists(file)) file = boost::filesystem::canonical(file).string();
    cout << " save " << file << endl;
    auto scene = VRScene::getCurrent();
    if (scene == 0) return;

    xmlpp::Document doc;
    xmlpp::Element* sceneN = doc.create_root_node("Scene", "", "VRF"); //name, ns_uri, ns_prefix
    xmlpp::Element* objectsN = sceneN->add_child("Objects");
    if (guiN) sceneN->import_node(guiN, true);

    // save scenegraph
    scene->setPath(file);
    VRObjectPtr root = scene->getRoot();
    root->saveUnder(objectsN);
    scene->saveScene(sceneN);
    doc.write_to_file_formatted(file);
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

    auto scene = VRScene::getCurrent();
    VRSceneLoader_current_scene = scene;

    scene->getRoot()->load(root);
    VRSceneManager::get()->setScene(scene);
    scene->loadScene(sceneN);
}


OSG_END_NAMESPACE;
