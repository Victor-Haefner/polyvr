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
    {"add", PyWrap(TextureMosaic, add, "add", void, VRTexturePtr, Vec2i, Vec2i) },
    //{"getChunkPosition", PyWrap(TextureMosaic, getChunkPosition, "getChunkPosition", Vec2i, Vec2i) },
    {"getChunkSize", PyWrap(TextureMosaic, getChunkSize, "getChunkSize", Vec2i, Vec2i) },
    {NULL}  /* Sentinel */
};



