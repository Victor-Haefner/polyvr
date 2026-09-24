#include "VRPyMenu.h"
#include "VRPyBaseT.h"

using namespace OSG;

template<> bool toValue(PyObject* obj, VRMenu::TYPE& e) {
    return toValue( PyUnicode_AsUTF8(obj) , e);
}

template<> bool toValue(PyObject* obj, VRMenu::LAYOUT& e) {
    return toValue( PyUnicode_AsUTF8(obj) , e);
}

simpleVRPyType(Menu, New_VRObjects_ptr);

PyMethodDef VRPyMenu::methods[] = {
    {"append", PyWrap( Menu, append, "Append a child menu - append(str texture_path)", VRMenuPtr, string ) },
    {"setLeafType", PyWrap( Menu, setLeafType, "Set menu layout - setLeafType(str type, vec2f scale)\n\ttype : ['SPRITE'], scale is the size of the sprite", void, VRMenu::TYPE, Vec2d ) },
    {"setLayout", PyWrap( Menu, setLayout, "Set menu layout - setLayout(str layout, float param)\n\tlayout : ['LINEAR', 'CIRCULAR'], param is the distance between leafs", void, VRMenu::LAYOUT, float ) },
    {"open", PyWrap( Menu, open, "Open menu", void ) },
    {"close", PyWrap( Menu, close, "Close menu", void ) },
    {"setCallback", PyWrap( Menu, setCallback, "Set a menu callback - setCallback(fkt, [params])", void, VRMenuCbPtr ) },
    {"trigger", PyWrap( Menu, trigger, "Trigger menu or enter next layer if no callback is set", void ) },
    {"move", PyWrap( Menu, move, "Move the cursor - move(int dir)\n\tleft: dir=-1, right: dir=1", void, int ) },
    {NULL}  /* Sentinel */
};
