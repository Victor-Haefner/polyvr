#ifndef VRGDAL_H_INCLUDED
#define VRGDAL_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>
#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

VRTexturePtr loadGeoRasterData(string path);

void loadPDF(string path, VRTransformPtr res);
void loadSHP(string path, VRTransformPtr res);
void loadTIFF(string path, VRTransformPtr res);
//void writeSHP(VRGeometryPtr geo, string path);

OSG_END_NAMESPACE;

#endif // VRGDAL_H_INCLUDED
