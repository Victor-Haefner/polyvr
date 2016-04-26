#include "VRVisualLayer.h"
#include "core/objects/object/VRObject.h"
#include "core/utils/VRFunction.h"

#include <OpenSG/OSGNode.h>

OSG_BEGIN_NAMESPACE;

VRVisualLayer::VRVisualLayer(string name, string ic) {
    setNameSpace("VRVisualLayer");
    if (layers.count(name)) layers.erase(name);
    setName(name);
    icon = ic;

    anchor = VRObject::create("layer_anchor_"+name);
    anchor->hide();
}

VRVisualLayer::~VRVisualLayer() {}

VRVisualLayerPtr VRVisualLayer::getLayer(string l, string icon, bool create) {
    if (layers.count(l)) if (auto ly = layers[l].lock()) return ly;
    if (!create) return 0;
    auto ly = VRVisualLayerPtr(new VRVisualLayer(l, icon) );
    layers[l] = ly;
    return ly;
}

string VRVisualLayer::getIconName() { return icon; }

map<string, VRVisualLayerWeakPtr> VRVisualLayer::layers;
vector<string> VRVisualLayer::getLayers() {
    vector<string> ls;
    for (auto l : layers) ls.push_back(l.first);
    return ls;
}

void VRVisualLayer::anchorLayers(VRObjectPtr root) {
    for (auto l : layers) {
        if (auto ly = l.second.lock()) {
            root->addChild( ly->anchor->getNode() );
        }
    }
}

void VRVisualLayer::clearLayers() { layers.clear(); }

void VRVisualLayer::setVisibility(bool b) { anchor->setVisible(b); if (auto sp = callback.lock()) (*sp)(b); }
bool VRVisualLayer::getVisibility() { return anchor->isVisible(); }

void VRVisualLayer::addObject(VRObjectPtr obj) { anchor->addChild(obj); }

void VRVisualLayer::setCallback(VRToggleWeakPtr fkt) { callback = fkt; }

OSG_END_NAMESPACE;


