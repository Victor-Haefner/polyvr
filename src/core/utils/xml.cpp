#include "xml.h"

#include <libxml/tree.h>


XMLElement::XMLElement(_xmlNode* node) : node(node) {}
XMLElement::~XMLElement() {}

XMLElementPtr XMLElement::create(_xmlNode* node) { return XMLElementPtr( new XMLElement(node) ); }

string XMLElement::getName() { return string((const char*)node->name); }
string XMLElement::getNameSpace() { return string((const char*)node->ns); }

_xmlNode* getNextNode(_xmlNode* cur) {
    while ( cur && xmlIsBlankNode(cur) ) cur = cur->next;
    return cur;
}

vector<XMLElementPtr> XMLElement::getChildren() {
    vector<XMLElementPtr> res;
    auto cnode = getNextNode( node->xmlChildrenNode );
    while (cnode) {
        res.push_back(XMLElement::create(cnode));
        cnode = getNextNode( cnode->next );
    }
    return res;
}


XML::XML() {}
XML::~XML() {}

XMLPtr XML::create() { return XMLPtr( new XML() ); }

void XML::read(string path) {
    xmlDocPtr doc = xmlParseFile(path.c_str());
    xmlNodePtr xmlRoot = xmlDocGetRootElement(doc);
    root = XMLElement::create(xmlRoot);

    xmlFreeDoc(doc);
}

void XML::write(string path) { // TODO
    ;
}

XMLElementPtr XML::getRoot() { return root; }




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
