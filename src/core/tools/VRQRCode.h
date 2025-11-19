#ifndef VRQRCODE_H_INCLUDED
#define VRQRCODE_H_INCLUDED

#include <string>
#include <OpenSG/OSGColor.h>
#include "core/math/OSGMathFwd.h"
#include "core/objects/VRObjectFwd.h"
#include "core/objects/material/VRMaterialFwd.h"

void createQRCode(std::string s, OSG::VRMaterialPtr mat, OSG::Color3f fg, OSG::Color3f bg, int offset);

#endif // VRQRCODE_H_INCLUDED
