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
#include "core/utils/system/VRSystem.h"
#include "core/utils/VREncryption.h"
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

#include "import/VRImport.h"

using namespace std;
using namespace OSG;

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

void VRSceneLoader::saveScene(string file, xmlpp::Element* guiN, string encryptionKey) {
    if (encryptionKey != "" && file[file.size()-1] == 'r') file[file.size()-1] = 'c';

    if (exists(file)) file = canonical(file);
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

    // write to file
    if (encryptionKey == "") doc.write_to_file_formatted(file);
#ifndef NO_ENCRYPTION
    else {
        VREncryptionPtr e = VREncryption::create();
        string data = doc.write_to_string_formatted();
        data = e->encrypt(data, encryptionKey, "0000000000000000");
        ofstream f(file, ios::binary);
        f.write( data.c_str(), data.size() );
        f.close();
    }
#endif
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

void VRSceneLoader::loadScene(string path, string encryptionKey) {
    xmlpp::DomParser parser;
    parser.set_validate(false);
    if (!exists(path)) return;

    if (encryptionKey== "")  parser.parse_file(path.c_str());
#ifndef NO_ENCRYPTION
     else {
        auto e = VREncryption::create();
        ifstream f(path, ios::binary);
        stringstream strm;
        strm << f.rdbuf();
        string data = e->decrypt(strm.str(), encryptionKey, "0000000000000000");
        if (data == "") {
            cout << "ERROR: loading scene " << path << " failed during decryption attempt, try load anyway!\n";
            parser.parse_file(path.c_str());
        } else {
            //data.pop_back(); // remove last char
            stringstream strm2(data);
            parser.parse_stream( strm2 );
        }
        f.close();
    }
#endif

    xmlpp::Node* n = parser.get_document()->get_root_node();
    xmlpp::Element* sceneN = dynamic_cast<xmlpp::Element*>(n);
    xmlpp::Element* objectsN = VRSceneLoader_getElementChild_(sceneN, "Objects");
    xmlpp::Element* root = VRSceneLoader_getElementChild_(objectsN, 0);

    auto scene = VRScene::getCurrent();
    scene->getRoot()->load(root);
    scene->loadScene(sceneN);
    VRSceneManager::get()->setScene(scene);
}

VRObjectPtr VRSceneLoader::importScene(string path, string encryptionKey, bool offLights) {
    xmlpp::DomParser parser;
    parser.set_validate(false);
    if (!exists(path)) return 0;

    if (encryptionKey== "")  parser.parse_file(path.c_str());
#ifndef NO_ENCRYPTION
     else {
        auto e = VREncryption::create();
        ifstream f(path, ios::binary);
        stringstream strm;
        strm << f.rdbuf();
        string data = e->decrypt(strm.str(), encryptionKey, "0000000000000000");
        if (data == "") {
            cout << "ERROR: loading scene " << path << " failed during decryption attempt, try load anyway!\n";
            parser.parse_file(path.c_str());
        } else {
            //data.pop_back(); // remove last char
            stringstream strm2(data);
            parser.parse_stream( strm2 );
        }
        f.close();
    }
#endif

    xmlpp::Node* n = parser.get_document()->get_root_node();
    xmlpp::Element* sceneN = dynamic_cast<xmlpp::Element*>(n);
    xmlpp::Element* objectsN = VRSceneLoader_getElementChild_(sceneN, "Objects");
    xmlpp::Element* root = VRSceneLoader_getElementChild_(objectsN, 0);

    auto scene = VRScene::getCurrent();
    auto rootNode = VRObject::create("sceneProxy");
    rootNode->load(root);
    rootNode->setPersistency(0);
    scene->importScene(sceneN);
    scene->getRoot()->addChild(rootNode);

    if (offLights) {
        for (auto obj : rootNode->getChildren(true, "Light")) {
            auto light = dynamic_pointer_cast<VRLight>(obj);
            if (light) light->setOn(false);
        }
    }

    return rootNode;
}
