#ifndef VRQRCODE_H_INCLUDED
#define VRQRCODE_H_INCLUDED

#include <string>
#include <OpenSG/OSGVector.h>
#include "core/objects/VRObjectFwd.h"

void createQRCode(std::string s, OSG::VRMaterialPtr mat, OSG::Vec3d fg, OSG::Vec3d bg, int offset);

#endif // VRQRCODE_H_INCLUDED
