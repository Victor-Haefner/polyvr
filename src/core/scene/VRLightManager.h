#ifndef VRLIGHTMANAGER_H_INCLUDED
#define VRLIGHTMANAGER_H_INCLUDED

#include "VRDefShading.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRLight;

class VRLightManager : public VRDefShading {
    protected:
        map<int, VRLight*> light_map;
        //VRObject* toplight;
        //VRObject* bottomlight;

    public:
        VRLightManager();

        VRLight* addLight(string name);

        VRLight* getLight(int ID);
};

OSG_END_NAMESPACE;

#endif // VRLIGHTMANAGER_H_INCLUDED
