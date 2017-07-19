#include "VRPySprite.h"
#include "VRPyTransform.h"
#include "VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(Sprite, New_VRObjects_ptr);

PyMethodDef VRPySprite::methods[] = {
    {"getText", PyWrap(Sprite, getLabel, "Get label text from sprite.", string) },
    {"getSize", PyWrap(Sprite, getSize, "Get size of sprite.", Vec2d) },
    {"setText", PyWrapOpt(Sprite, setLabel, "Set label text from sprite.", "1", void, string, float ) },
    {"setSize", PyWrap(Sprite, setSize, "Set sprite size.", void, float, float ) },
    {"setTexture", PyWrap(Sprite, setTexture, "Set sprite texture", void, string) },
    {"webOpen", PyWrap(Sprite, webOpen, "Open and display a website - webOpen(str uri, int width, flt ratio)", void, string, int, float) },
    {"convertToCloth", PyWrap(Sprite, convertToCloth, "convert this Sprite to cloth (softbody)", void) },
    {NULL}  /* Sentinel */
};

