#ifndef VRMATERIALMANAGER_H_INCLUDED
#define VRMATERIALMANAGER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/utils/VRStorage.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRMaterialManager : public virtual VRStorage {
    private:

    public:
        VRMaterialManager();
        ~VRMaterialManager();

        void update();
};

OSG_END_NAMESPACE;

#endif // VRMATERIALMANAGER_H_INCLUDED
