#ifndef VRHMDDISTORTION_H_INCLUDED
#define VRHMDDISTORTION_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/objects/VRObjectFwd.h"
#include "core/objects/VRStage.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRHMDDistortion : public VRStage {
    private:
        VRMaterialPtr hmdd_mat;

    public:
        VRHMDDistortion();
        ~VRHMDDistortion();

        void initHMDD(VRMaterialPtr mat);
        void setHMDDparams(float radius);
        void reload();
};

OSG_END_NAMESPACE;

#endif // VRHMDDISTORTION_H_INCLUDED
