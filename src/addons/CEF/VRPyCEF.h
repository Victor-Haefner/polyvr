#ifndef VRPyCEF_H_INCLUDED
#define VRPyCEF_H_INCLUDED

#include "CEF.h"
#ifndef WITHOUT_IMGUI
#include "../Gui/VRGui.h"
#endif
#include "core/scripting/VRPyBase.h"

struct VRPyCEF : VRPyBaseT<OSG::CEF> {
    static PyMethodDef methods[];
};

#ifndef WITHOUT_IMGUI
struct VRPyGui : VRPyBaseT<OSG::VRGui> {
    static PyMethodDef methods[];
};
#endif

#endif // VRPyCEF_H_INCLUDED
