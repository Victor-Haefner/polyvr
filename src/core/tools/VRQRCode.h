#ifndef VRQRCODE_H_INCLUDED
#define VRQRCODE_H_INCLUDED

#include <string>
#include <OpenSG/OSGVector.h>

namespace OSG { class VRMaterial; }

void createQRCode(std::string s, OSG::VRMaterial* mat, OSG::Vec3f fg, OSG::Vec3f bg, int offset);

#endif // VRQRCODE_H_INCLUDED
