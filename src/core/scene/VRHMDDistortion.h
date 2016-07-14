#ifndef VRHMDDISTORTION_H_INCLUDED
#define VRHMDDISTORTION_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRHMDDistortion {
    private:
        VRMaterialPtr hmdd_mat;

    public:
        VRHMDDistortion();
        ~VRHMDDistortion();

        void initHMDD(VRMaterialPtr mat);
        void setHMDDparams(float radius);
};

OSG_END_NAMESPACE;

#endif // VRHMDDISTORTION_H_INCLUDED
