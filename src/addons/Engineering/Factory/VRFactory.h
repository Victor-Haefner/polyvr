#ifndef VRFACTORY_H_INCLUDED
#define VRFACTORY_H_INCLUDED

#include "../objects/VR3DEntity.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRFactory{
    public:
        struct workplace {
            workplace() : indMaterial(0), geo(0), indicator(0), nametag(0) {}

            //ecoflex
            int ID;
            int parentID;
            string name;
            float ecoindex;
            vector<int> inputnodes;
            vector<int> outputnodes;

            //visualisation
            bool placed;
            Vec3f pos;
            Vec3f dir;
            Vec3f up;
            string file;

            SimpleMaterialRecPtr indMaterial;
            VR3DEntity* geo;
            VRGeometry* indicator;
            VRGeometry* nametag;
            ImageRecPtr nameTexture;

            static float min_index;
            static float max_index;
        };

    public:
        VRFactory(string name) {

        }
};

float VRFactory::workplace::min_index = 1;
float VRFactory::workplace::max_index = 0;

OSG_END_NAMESPACE;

#endif // VRFACTORY_H_INCLUDED
