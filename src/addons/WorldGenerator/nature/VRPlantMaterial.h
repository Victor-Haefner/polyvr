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

        void composeShader();

    public:
        VRPlantMaterial();
        ~VRPlantMaterial();

        static VRPlantMaterialPtr create();

};

OSG_END_NAMESPACE;

#endif // VRPLANTMATERIAL_H_INCLUDED
