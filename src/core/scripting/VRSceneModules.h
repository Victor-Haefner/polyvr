#ifndef VRSCENEMODULES_H_INCLUDED
#define VRSCENEMODULES_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "VRPyBase.h"

OSG_BEGIN_NAMESPACE;

class VRScriptManager;

class VRSceneModules: public VRPyBase {
    private:
    public:
        void setup(VRScriptManager* sm, PyObject* pModVR);
};

OSG_END_NAMESPACE;

#endif // VRSCENEMODULES_H_INCLUDED
