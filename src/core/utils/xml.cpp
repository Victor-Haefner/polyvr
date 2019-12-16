#include "xml.h"

#include <iostream>
#include <libxml/tree.h>
#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>


XMLElement::XMLElement(_xmlNode* node) : node(node) {}
XMLElement::~XMLElement() {}

XMLElementPtr XMLElement::create(_xmlNode* node) { return XMLElementPtr( new XMLElement(node) ); }

string XMLElement::getName() { return string((const char*)node->name); }
string XMLElement::getNameSpace() { return string((const char*)node->ns); }

string XMLElement::getText() { return string((const char*)xmlNodeGetContent( node->children ) ); }

string XMLElement::getAttribute(string name) {
    xmlAttr* attribute = node->properties;
    while(attribute) {
        xmlChar* aname = xmlNodeListGetString(node->doc, attribute->children, 0);
        xmlChar* value = xmlNodeListGetString(node->doc, attribute->children, 1);
        string ans((const char*)aname);
        string vas((const char*)value);
        xmlFree(aname);
        xmlFree(value);
        if (ans == name) return vas;
        attribute = attribute->next;
    }
    return "";
}

bool XMLElement::hasAttribute(string name) {
    xmlAttr* attribute = node->properties;
    while(attribute) {
        xmlChar* aname = xmlNodeListGetString(node->doc, attribute->children, 0);
        string ans((const char*)aname);
        xmlFree(aname);
        if (ans == name) return true;
        attribute = attribute->next;
    }
    return false;
}

void XMLElement::setAttribute(string name, string value) {
    xmlSetProp(node, (xmlChar*)name.c_str(), (xmlChar*)value.c_str());
}

_xmlNode* getNextNode(_xmlNode* cur) {
    while ( cur && xmlIsBlankNode(cur) ) cur = cur->next;
    return cur;
}

vector<XMLElementPtr> XMLElement::getChildren(string name) {
    vector<XMLElementPtr> res;
    auto cnode = getNextNode( node->xmlChildrenNode );
    while (cnode) {
        if (name != "" && name != string((const char*)cnode->name)) continue;
        res.push_back(XMLElement::create(cnode));
        cnode = getNextNode( cnode->next );
    }
    return res;
}

XMLElementPtr XMLElement::getChild(string name) {
    auto cnode = getNextNode( node->xmlChildrenNode );
    while (cnode) {
        if (name == string((const char*)cnode->name)) return XMLElement::create(cnode);
        cnode = getNextNode( cnode->next );
    }
    return 0;
}

XMLElementPtr XMLElement::addChild(string name) {
    auto child = xmlNewNode(NULL, (xmlChar*)name.c_str());
    xmlAddChild(node, child);
    return XMLElement::create(child);
}

void XMLElement::setText(string text) {
    auto child = xmlNewText((xmlChar*)text.c_str());
    xmlAddChild(node, child);
}



XML::XML() {}
XML::~XML() {
    if (doc) xmlFreeDoc(doc);
}

XMLPtr XML::create() { return XMLPtr( new XML() ); }

void XML::read(string path) {
    if (doc) xmlFreeDoc(doc);
    doc = xmlParseFile(path.c_str());
    xmlNodePtr xmlRoot = xmlDocGetRootElement(doc);
    root = XMLElement::create(xmlRoot);
}

void XML::write(string path) { // TODO
    xmlTextWriterPtr writer = xmlNewTextWriterFilename(path.c_str(), 0);
}

XMLElementPtr XML::getRoot() { return root; }

XMLElementPtr XML::newRoot(string name, string ns_uri, string ns_prefix) {
    if (doc) xmlFreeDoc(doc);
    doc = xmlNewDoc(NULL);
    auto ns = xmlNewNs(NULL, (xmlChar*)ns_uri.c_str(), (xmlChar*)ns_prefix.c_str());
    auto rnode = xmlNewNode(ns, (xmlChar*)name.c_str());
    root = XMLElement::create( rnode );
    return root;
}

void XML::printTree(XMLElementPtr e, string D) {
    cout << D << e->getName() << endl;
    for (auto c : e->getChildren()) printTree(c, D + " ");
}



/**

src/addons/Engineering/Factory/VRAMLLoader.cpp      5|#include <libxml++/libxml++.h>|
src/addons/Semantics/Processes/VRProcessLayout.cpp  18|#include <libxml++/libxml++.h>|
src/addons/Semantics/Processes/VRProcessLayout.cpp  19|#include <libxml++/nodes/element.h>|
src/addons/Semantics/Reasoning/VROWLExport.cpp      5|#include <libxml++/libxml++.h>|
src/addons/Semantics/Reasoning/VROWLExport.cpp      6|#include <libxml++/nodes/element.h>|
src/addons/WorldGenerator/GIS/OSMMap.cpp            5|#include <libxml++/libxml++.h>|
src/core/gui/VRGuiScripts.cpp                       27|#include <libxml++/nodes/element.h>|
src/core/gui/VRGuiScripts.cpp                       28|#include <libxml++/libxml++.h>|
src/core/networking/VRSocket.cpp                    17|#include <libxml++/nodes/element.h>|
src/core/objects/VRCamera.cpp                       16|#include <libxml++/nodes/element.h>|
src/core/objects/VRGroup.cpp                        3|#include <libxml++/nodes/element.h>|
src/core/objects/VRLightBeacon.cpp                  10|#include <libxml++/nodes/element.h>|
src/core/objects/VRLod.cpp                          7|#include <libxml++/nodes/element.h>|
src/core/objects/geometry/VRGeometry.cpp            3|#include <libxml++/nodes/element.h>|
src/core/objects/material/VRMaterial.cpp            45|#include <libxml++/nodes/element.h>|
src/core/scene/VRScene.cpp                          24|#include <libxml++/nodes/element.h>|
src/core/scene/VRSceneLoader.cpp                    23|#include <libxml++/libxml++.h>|
src/core/scene/VRSceneLoader.cpp                    24|#include <libxml++/nodes/element.h>|
src/core/scripting/VRScript.cpp                     25|#include <libxml++/nodes/element.h>|
src/core/scripting/VRScript.cpp                     26|#include <libxml++/nodes/textnode.h>|
src/core/setup/VRSetup.cpp                          20|#include <libxml++/libxml++.h>|
src/core/setup/VRSetup.cpp                          21|#include <libxml++/nodes/element.h>|
src/core/setup/devices/VRDevice.cpp                 6|#include <libxml++/nodes/element.h>|
src/core/setup/devices/VRDeviceManager.cpp          6|#include <libxml++/nodes/element.h>|
src/core/setup/tracking/ART.cpp                     8|#include <libxml++/nodes/element.h>|
src/core/setup/tracking/VRPN.cpp                    12|#include <libxml++/nodes/element.h>|
src/core/setup/windows/VRMultiWindow.cpp            7|#include <libxml++/nodes/element.h>|
src/core/setup/windows/VRView.cpp                   4|#include <libxml++/nodes/element.h>|
src/core/setup/windows/VRWindow.cpp                 12|#include <libxml++/nodes/element.h>|
src/core/setup/windows/VRWindowManager.cpp          5|#include <libxml++/nodes/element.h>|
src/core/tools/VRProjectManager.cpp                 6|#include <libxml++/libxml++.h>|
src/core/tools/VRProjectManager.cpp                 7|#include <libxml++/nodes/element.h>|
src/core/utils/VRName.cpp                           9|#include <libxml++/nodes/element.h>|
src/core/utils/VRStorage.cpp                        6|#include <libxml++/libxml++.h>|
src/core/utils/VRStorage.cpp                        7|#include <libxml++/nodes/element.h>|
src/core/utils/VRStorage_template.h                 4|#include <libxml++/nodes/element.h>|


*/
