#ifndef VRPLANTMATERIAL_H_INCLUDED
#define VRPLANTMATERIAL_H_INCLUDED

#include "core/objects/material/VRMaterial.h"
#include "addons/WorldGenerator/VRWorldGeneratorFwd.h"

OSG_BEGIN_NAMESPACE;

class VRPlantMaterial : public VRMaterial {
    private:
        static string vShrdHead;
        static string fShrdHead;
        static string vShrdEnd;
        static string fShrdEnd;
        static string lightning;
        static string noLightning;

    public:
        VRPlantMaterial();
        ~VRPlantMaterial();

        static VRPlantMaterialPtr create();

        void composeShader();
};

OSG_END_NAMESPACE;

#endif // VRPLANTMATERIAL_H_INCLUDED
