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
#include "core/utils/xml.h"
#include "core/utils/VRTimer.h"
#include "core/utils/VRStorage_template.h"
#include "core/utils/system/VRSystem.h"
#include "core/utils/VREncryption.h"
#include "core/navigation/VRNavigator.h"
#include "core/objects/VRLod.h"
#include "addons/construction/building/VROpening.h"

#include <OpenSG/OSGGeometry.h>
#include <OpenSG/OSGSimpleMaterial.h>
#include <OpenSG/OSGPointLight.h>
#include <OpenSG/OSGGeoProperties.h>
#include <OpenSG/OSGComponentTransform.h>
#include <OpenSG/OSGNameAttachment.h>

#include <stdio.h>
#include <fstream>
#ifndef _WIN32
#include <unistd.h>
#endif

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

    for (unsigned int i=0;i<obj->getChildrenCount();i++)
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
VRSceneLoader::~VRSceneLoader() { cout << "VRSceneLoader::~VRSceneLoader\n"; }

VRSceneLoader* VRSceneLoader::get() {
    static VRSceneLoader* s = new VRSceneLoader();
    return s;
}

void VRSceneLoader::saveScene(string file, XMLElementPtr guiN, string encryptionKey) {
    if (encryptionKey != "" && file[file.size()-1] == 'r') file[file.size()-1] = 'c';

    if (exists(file)) file = canonical(file);
    cout << " save " << file << endl;
    auto scene = VRScene::getCurrent();
    if (scene == 0) return;

    XML xml;
    XMLElementPtr sceneN = xml.newRoot("Scene", "", ""); //name, ns_uri, ns_prefix
    XMLElementPtr objectsN = sceneN->addChild("Objects");
    if (guiN) sceneN->importNode(guiN, true, xml);

    // save scenegraph
    scene->setPath(file);
    VRObjectPtr root = scene->getRoot();
    root->saveUnder(objectsN);
    scene->saveScene(sceneN);

    // write to file
    if (encryptionKey == "") xml.write(file);
#ifndef WITHOUT_CRYPTOPP
    else {
        VREncryptionPtr e = VREncryption::create();
        string data = xml.toString();
        data = e->encrypt(data, encryptionKey, "0000000000000000");
        ofstream f(file, ios::binary);
        f.write( data.c_str(), data.size() );
        f.close();
    }
#endif
}

/*XMLElementPtr VRSceneLoader_getElementChild_(XMLElementPtr e, string name) {
    if (e == 0) return 0;
    xmlpp::Node::NodeList nl = e->get_children();
    xmlpp::Node::NodeList::iterator itr;
    for (itr = nl.begin(); itr != nl.end(); itr++) {
        xmlpp::Node* n = *itr;

        XMLElementPtr el = dynamic_cast<XMLElementPtr>(n);
        if (!el) continue;

        if (el->get_name() == name) return el;
    }
    return 0;
}

XMLElementPtr VRSceneLoader_getElementChild_(XMLElementPtr e, int i) {
    if (e == 0) return 0;
    xmlpp::Node::NodeList nl = e->get_children();
    xmlpp::Node::NodeList::iterator itr;
    int j = 0;
    for (itr = nl.begin(); itr != nl.end(); itr++) {
        xmlpp::Node* n = *itr;
        XMLElementPtr el = dynamic_cast<XMLElementPtr>(n);
        if (!el) continue;

        if (i == j) return el;
        j++;
    }

    return 0;
}*/

bool VRSceneLoader::loadScene(string path, string encryptionKey) {
    cout << "VRSceneLoader::loadScene " << path << endl;
    XML xml;
    if (!exists(path)) return false;

    if (encryptionKey== "")  xml.read(path, false);
#ifndef WITHOUT_CRYPTOPP
    else {
        auto e = VREncryption::create();
        ifstream f(path, ios::binary);
        stringstream strm;
        strm << f.rdbuf();
        string data = e->decrypt(strm.str(), encryptionKey, "0000000000000000");
        if (data == "") {
            cout << "ERROR: loading scene " << path << " failed during decryption attempt, try load anyway!\n";
            xml.read(path, false);
        } else xml.parse(data, false);
        f.close();
    }
#endif

    XMLElementPtr sceneN = xml.getRoot();
    XMLElementPtr objectsN = sceneN->getChild("Objects");
    if (!objectsN) return false;
    XMLElementPtr root = objectsN->getChild(0);

    auto scene = VRScene::getCurrent();
    scene->getRoot()->load(root);
    scene->loadScene(sceneN);
    VRSceneManager::get()->setScene(scene);
    cout << " VRSceneLoader::loadScene done" << endl;
    return true;
}

VRObjectPtr VRSceneLoader::importScene(string path, string encryptionKey, bool offLights) {
    XML xml;
    if (!exists(path)) return 0;

    if (encryptionKey== "")  xml.read(path, false);
#ifndef WITHOUT_CRYPTOPP
    else {
        auto e = VREncryption::create();
        ifstream f(path, ios::binary);
        stringstream strm;
        strm << f.rdbuf();
        string data = e->decrypt(strm.str(), encryptionKey, "0000000000000000");
        if (data == "") {
            cout << "ERROR: loading scene " << path << " failed during decryption attempt, try load anyway!\n";
            xml.read(path, false);
        } else xml.parse(data, false);
        f.close();
    }
#endif

    XMLElementPtr sceneN = xml.getRoot();
    XMLElementPtr objectsN = sceneN->getChild("Objects");
    XMLElementPtr root = objectsN->getChild(0);

    auto scene = VRScene::getCurrent();
    auto rootNode = VRObject::create("sceneProxy");
    rootNode->load(root);
    rootNode->setPersistency(0);
    scene->importScene(sceneN, path);
    scene->getRoot()->addChild(rootNode);

    if (offLights) {
        for (auto obj : rootNode->getChildren(true, "Light")) {
            auto light = dynamic_pointer_cast<VRLight>(obj);
            if (light) light->setOn(false);
        }
    }

    return rootNode;
}
