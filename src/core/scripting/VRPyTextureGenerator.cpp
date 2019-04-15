#include "VRPyTextureGenerator.h"
#include "VRPyGeometry.h"
#include "VRPyDevice.h"
#include "VRPyBaseT.h"
#include "VRPyImage.h"
#include "VRPyPath.h"

#include "core/math/path.h"

using namespace OSG;

simpleVRPyType(TextureGenerator, New_ptr);

PyMethodDef VRPyTextureGenerator::methods[] = {
    {"add", PyWrap( TextureGenerator, add, "Add a layer\n\ttype can be: 'Perlin', 'Bricks'", void, string, float, Color4f, Color4f ) },
    {"drawFill", PyWrap( TextureGenerator, drawFill, "Fill whole texture", void, Color4f ) },
    {"drawPixel", PyWrap( TextureGenerator, drawPixel, "Set a pixel color", void, Vec3i, Color4f ) },
    {"drawLine", PyWrap( TextureGenerator, drawLine, "Add a line in UV space", void, Vec3d, Vec3d, Color4f, float ) },
    {"drawPath", PyWrap( TextureGenerator, drawPath, "Add a path in UV space", void, PathPtr, Color4f, float ) },
    {"setSize", PyWrapOpt( TextureGenerator, setSize, "Set the size\n   set depth to 1 for 2D textures", "0", void, Vec3i, bool ) },
    {"getSize", PyWrap( TextureGenerator, getSize, "Get the size", Vec3i ) },
    {"compose", PyWrapOpt( TextureGenerator, compose, "Bake the layers into an image, optional seed", "0", VRTexturePtr, int ) },
    {"read", PyWrap( TextureGenerator, read, "Read an image from file", void, string ) },
    {"readSharedMemory", PyWrap( TextureGenerator, readSharedMemory, "Read an image from shared memory, segment, data", VRTexturePtr, string, string ) },
    {"addSimpleNoise", PyWrapOpt( TextureGenerator, addSimpleNoise, "Add simple noise based on front and backcolor, texture dimension, alpha channel, fg, bg, amount", "1", void, Vec3i, bool, Color4f, Color4f, float) },
    {NULL}  /* Sentinel */
};
