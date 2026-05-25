#ifndef VRPYMENU_H_INCLUDED
#define VRPYMENU_H_INCLUDED

#include "VRPyBase.h"
#include "core/tools/VRMenu.h"

struct VRPyMenu : VRPyBaseT<OSG::VRMenu> {
    static PyMethodDef methods[];
};

#endif // VRPYMENU_H_INCLUDED
