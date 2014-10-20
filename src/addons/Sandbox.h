#ifndef SANDBOX_H_INCLUDED
#define SANDBOX_H_INCLUDED

#include "core/scene/VRScene.h"
#include "core/VRSceneLoader.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class Sandbox {
    private:
        VRScene* scene;
    public:
        Sandbox() {
            scene = VRSceneLoader::get()->parseSceneFromXML("scene/sandbox.xml", "Sandbox");
        }
};

OSG_END_NAMESPACE

#endif // SANDBOX_H_INCLUDED
