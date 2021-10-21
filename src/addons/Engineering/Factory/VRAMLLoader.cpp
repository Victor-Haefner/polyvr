#include "VRAMLLoader.h"
#include "core/utils/toString.h"
#include "core/utils/xml.h"
#include "core/objects/geometry/VRGeometry.h"

#include <iostream>
#include <OpenSG/OSGVector.h>

using namespace OSG;

VRAMLLoader::VRAMLLoader() {}
VRAMLLoader::~VRAMLLoader() {}

VRAMLLoaderPtr VRAMLLoader::create()  { return VRAMLLoaderPtr(new VRAMLLoader()); }

VROntologyPtr VRAMLLoader::getOntology() { return ontology; }
VRTransformPtr VRAMLLoader::getScene() { return scene; }

void VRAMLLoader::read(string path) {
    XML xml;
    xml.read(path);

    scene = VRTransform::create("scene");
    auto hierarchy = xml.getRoot()->getChild("InstanceHierarchy"); // scene graph
	readNode(hierarchy, scene);
}

void VRAMLLoader::readNode(XMLElementPtr node, VRObjectPtr parent) {
    if (!node) return;

    processElement(node, parent);

    string name = node->getName();
    if (name == "InstanceHierarchy" || name == "InternalElement") {
        auto child = VRTransform::create( node->getAttribute("Name") );
        parent->addChild(child);
        for (auto childNode : node->getChildren()) readNode(childNode, child);
    }
}

void VRAMLLoader::processElement(XMLElementPtr node, VRObjectPtr parent) {
    string name = node->getName();

    // geometries
    /*if (name == "ExternalInterface" && node->getAttribute("Name") == "Representation") {
        for (a : node->getChildren('Attribute'))
            if a.getAttribute('Name') == 'refType':
                ref = a.getChild('Value').getText()
                if ref == 'implicit': parent.hide() # 7.6 10^6
            if a.getAttribute('Name') == 'refURI':
                uri = a.getChild('Value').getText()
                if '.dae' in uri:
                    scene = ''
                    if '#' in uri:
                        scene = uri.split('#')[1]
                        uri = uri.split('#')[0]
                    if not uri in assets:
                        assets[uri] = VR.loadGeometry(path+uri, preset = 'COLLADA', options = { 'scene' : scene })
                    #if len(assets) == 56:
                    parent.addChild(assets[uri].duplicate())
                    print uri
    }

    # transformation
    if n.getName() == 'Attribute' and n.getAttribute('Name') == 'Frame':
        pos = [0,0,0]
        eul = [0,0,0]
        for a in n.getChildren('Attribute'):
            if a.getAttribute('Name') == 'x': pos[0] = float(a.getChild('Value').getText())*1000
            if a.getAttribute('Name') == 'y': pos[1] = float(a.getChild('Value').getText())*1000
            if a.getAttribute('Name') == 'z': pos[2] = float(a.getChild('Value').getText())*1000
            if a.getAttribute('Name') == 'rx': eul[0] = float(a.getChild('Value').getText())*pi/180.0
            if a.getAttribute('Name') == 'ry': eul[1] = float(a.getChild('Value').getText())*pi/180.0
            if a.getAttribute('Name') == 'rz': eul[2] = float(a.getChild('Value').getText())*pi/180.0
        parent.setFrom(pos)
        parent.setEuler(eul)
        #print pos, eul

    # role
    if n.getName() == 'RoleRequirements':
        role = n.getAttribute('RefBaseRoleClassPath')
        role = role.split('/')[-1]
        if not VR.ontology.getConcept(role):
            VR.ontology.addConcept(role)
        e = VR.ontology.addEntity('e', role)
        parent.setEntity(e)*/
}

void VRAMLLoader::write(string path) {

}


