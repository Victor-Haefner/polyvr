#include "VRPySelector.h"
#include "VRPySelection.h"
#include "VRPyObject.h"
#include "VRPyBaseT.h"
#include "VRPyTypeCaster.h"

using namespace OSG;

template<> bool toValue(PyObject* obj, VRSelector::VISUAL& e) {
    return toValue( PyUnicode_AsUTF8(obj) , e);
}

simpleVRPyType(Selector, New_ptr);

PyMethodDef VRPySelector::methods[] = {
    {"setVisual", PyWrap( Selector, setVisual, "Set visualization type, 'OUTLINE' or 'OVERLAY'", void, VRSelector::VISUAL ) },
    {"setColor", PyWrapOpt( Selector, setColor, "Set the color of the selection", "1", void, Color3f, float ) },
    {"deselect", PyWrap( Selector, clear, "Deselect object", void ) },
    {"select", PyWrapOpt( Selector, select, "Select object - select( obj, add, recursive )", "0|1", void, VRObjectPtr, bool, bool ) },
    {"update", PyWrap( Selector, update, "Update selection visualisation", void ) },
    {"set", PyWrap( Selector, set, "Set selection", void, VRSelectionPtr ) },
    {"add", PyWrap( Selector, add, "Add to selection", void, VRSelectionPtr ) },
    {"clear", PyWrap( Selector, clear, "Clear selection", void ) },
    {"getSelected", PyWrap( Selector, getSelected, "Return the selected object", VRObjectPtr ) },
    {"getSelection", PyWrap( Selector, getSelection, "Return the selection", VRSelectionPtr ) },
    {"setBorder", PyWrapOpt( Selector, setBorder, "Set the border width and toggles smoothness (width, smooth)", "1", void, int, bool ) },
    {NULL}  /* Sentinel */
};
