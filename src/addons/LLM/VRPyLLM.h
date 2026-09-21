#ifndef VRPYLLM_H_INCLUDED
#define VRPYLLM_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "VRLLM.h"
#include "core/scripting/VRPyBase.h"

struct VRPyLLM : VRPyBaseT<OSG::VRLLM> {
    static PyMethodDef methods[];
};

#endif //VRPYLLM_H_INCLUDED
