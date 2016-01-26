#ifndef VRSSAO_H_INCLUDED
#define VRSSAO_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRSSAO {
    private:
        VRMaterialPtr ssao_mat;

    public:
        VRSSAO();
        ~VRSSAO();

        void initSSAO(VRMaterialPtr mat);
        void setSSAOparams(float radius, int kernel, int noise);
};

OSG_END_NAMESPACE;

#endif // VRSSAO_H_INCLUDED
