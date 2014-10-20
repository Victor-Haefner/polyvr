#include "VRLightManager.h"
#include "core/objects/VRLight.h"
#include "core/utils/VROptions.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

VRLightManager::VRLightManager() {
    //toplight = new VRObject("lights_top");
    //bottomlight = new VRObject("lights_bottom");

    cout << "Init VRLightManager\n";
    //toplight->addChild(bottomlight);

    //if ( VROptions::get()->getOption<bool>("deferredShading") ) initDeferredShading(toplight);
}

VRLight* VRLightManager::addLight(string name) {
    VRLight* l = new VRLight(name);
    light_map[l->getID()] = l;
    return l;
}

VRLight* VRLightManager::getLight(int ID) {
    return light_map[ID];
}

OSG_END_NAMESPACE;
