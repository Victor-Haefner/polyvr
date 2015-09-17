#include "VRVisualLayer.h"
#include "core/objects/object/VRObject.h"
#include "core/utils/VRFunction.h"

#include <OpenSG/OSGNode.h>

OSG_BEGIN_NAMESPACE;

VRVisualLayer::VRVisualLayer(string name, string ic) {
    setNameSpace("VRVisualLayer");
    if (layers.count(name)) delete layers[name];
    setName(name);

    layers[name] = this;
    anchor = new VRObject("layer_anchor_"+name);
    anchor->hide();

    icon = ic;
}

VRVisualLayer::~VRVisualLayer() {
    delete anchor;
}

string VRVisualLayer::getIconName() { return icon; }

map<string, VRVisualLayer*> VRVisualLayer::layers;
vector<string> VRVisualLayer::getLayers() {
    vector<string> ls;
    for (auto l : layers) ls.push_back(l.first);
    return ls;
}

void VRVisualLayer::anchorLayers(VRObject* root) {
    //for (auto l : layers) l.second->anchor->switchParent(root);
    for (auto l : layers) root->addChild( l.second->anchor->getNode() );
}

VRVisualLayer* VRVisualLayer::getLayer(string l) { return layers.count(l) ? layers[l] : 0; }
void VRVisualLayer::clearLayers() { layers.clear(); }

void VRVisualLayer::setVisibility(bool b) { anchor->setVisible(b); if (auto sp = callback.lock()) (*sp)(b); }
bool VRVisualLayer::getVisibility() { return anchor->isVisible(); }

void VRVisualLayer::addObject(VRObject* obj) { anchor->addChild(obj); }

void VRVisualLayer::setCallback(VRToggleWeakPtr fkt) { callback = fkt; }

OSG_END_NAMESPACE;


