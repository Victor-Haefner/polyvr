#ifndef VRSSAO_H_INCLUDED
#define VRSSAO_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/objects/VRObjectFwd.h"
#include "core/objects/material/VRMaterialFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRSSAO {
    private:
        VRMaterialPtr ssao_mat;
        VRMaterialPtr blur_matX;
        VRMaterialPtr blur_matY;

        float radius = 0.03;
        int kernelSize = 6;
        int noiseSize = 6;

    public:
        VRSSAO();
        ~VRSSAO();

        void initSSAO(VRMaterialPtr mat);
        void initBlur(VRMaterialPtr matX, VRMaterialPtr matY);
        void setSSAOparams(float radius, int kernel, int noise);
        void updateShader();
};

OSG_END_NAMESPACE;

#endif // VRSSAO_H_INCLUDED
