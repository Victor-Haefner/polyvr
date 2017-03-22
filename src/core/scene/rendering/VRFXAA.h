#ifndef VRFXAAISTORTION_H_INCLUDED
#define VRFXAAISTORTION_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/objects/VRObjectFwd.h"
#include "core/objects/VRStage.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRFXAA : public VRStage {
    private:
        VRMaterialPtr fxaa_mat;

    public:
        VRFXAA();
        ~VRFXAA();

        void initFXAA(VRMaterialPtr mat);
        void setFXAAparams(float eye);
        void reload();
};

OSG_END_NAMESPACE;

#endif // VRFXAAISTORTION_H_INCLUDED
