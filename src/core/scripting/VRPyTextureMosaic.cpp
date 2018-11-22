#include "VRPyTextureMosaic.h"
#include "VRPyImage.h"

#include "VRPyBaseT.h"
#include "VRPyBaseFactory.h"

#include "core/objects/object/VRObject.h"
#include "core/objects/material/VRTexture.h"
#include "core/objects/VRObjectFwd.h"
#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGVector.h>
#include "VRPyMath.h"
#include <OpenSG/OSGGeoProperties.h>
#include <OpenSG/OSGGeometry.h>

using namespace OSG;

simpleVRPyType(TextureMosaic, New_ptr);

PyMethodDef VRPyTextureMosaic::methods[] = {
    {"add", PyWrap(TextureMosaic, add, "add image to mosaic, img, pos, id", void, VRTexturePtr, Vec2i, Vec2i) },
    {"getChunkPosition", PyWrap(TextureMosaic, getChunkPosition, "return chunk position", Vec2i, Vec2i) },
    {"getChunkSize", PyWrap(TextureMosaic, getChunkSize, "return chunk size", Vec2i, Vec2i) },
    {"getChunkUVasVector", PyWrap(TextureMosaic, getChunkUVasVector, "returns vector of UV coords in a list", vector<Vec2d>, Vec2i) },
    {"getChunkUV", PyWrap(TextureMosaic, getChunkUV, "returns UVmin and UVmax", Vec4d, Vec2i) },
    {NULL}  /* Sentinel */
};



