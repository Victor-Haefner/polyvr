#ifndef VRGDAL_H_INCLUDED
#define VRGDAL_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>
#include <vector>
#include <map>
#include "core/objects/VRObjectFwd.h"
#include "core/objects/material/VRMaterialFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

VRTexturePtr processGeoRasterData(string data, string driver, bool shout = true, float *heightoffset = NULL);
VRTexturePtr loadGeoRasterData(string path, bool shout = true, float *heightoffset = NULL);

void loadPDF(string path, VRTransformPtr res, map<string, string> opts);
void loadSHP(string path, VRTransformPtr res, map<string, string> opts);
void loadTIFF(string path, VRTransformPtr res, map<string, string> opts);
void writeGeoRasterData(string path, VRTexturePtr tex, double geoTransform[6], string params[3]);
void divideTiffIntoChunks(string pathIn, string pathOut, double minLat, double maxLat, double minLon, double maxLon, double res);
void divideTiffIntoChunksEPSG(string pathIn, string pathOut, double minEasting, double maxEasting, double minNorthing, double maxNorthing, double pixelResolution, double chunkResolution, bool debug = false);
vector<double> getGeoTransform(string path);
//void writeSHP(VRGeometryPtr geo, string path);

OSG_END_NAMESPACE;

#endif // VRGDAL_H_INCLUDED
