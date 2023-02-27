#include "VRShaderFactory.h"
#include "VRShader.h"

using namespace OSG;

/** Method

1) Meta Definition of shader, in multiple layers

Tasks:
    1) First layer, very close to GLSL but abstract the version
        a) Compute the min GLSL version based on the layer1 shader
        b) Define declarations based on GLSL version
        c) Generate the GLSL code

Version depending GLSL changes:
    Basic Version: 120
    Posible Versions: 120, 130, 140, 150, 330, 400, 410 .. 460

    1) 130+
        in and out are used instead of attribute and varying
        int and uint support (and bitwise operations with them)
        switch statement support
        New built-ins: trunc, round, roundEven, isnan, isinf, modf, acosh, asinh, cosh, gl_VertexID, inversesqrt, sign, sinh,
            smoothstep, tanh, texelFetch, texelFetchOffset, textureGrad, textureGradOffset, textureLod, textureLodOffset
            textureOffset, textureProj, textureProjGrad, textureProjGradOffset, textureProjLod, textureProjLodOffset,
            textureProjOffset, textureSize
        Fragment output can be user-defined
        gl_ClipDistance in vertex shader

    2) 140+
        New built-ins: inverse, gl_InstanceID

    3) 150+
        texture() should now be used instead of texture2D()
        New built-ins: determinant, EmitVertex, EndPrimitive, gl_InvocationID, gl_PrimitiveID, gl_PrimitiveIDIn, gl_ViewportIndex
        gl_ClipDistance, gl_Layer in geometry shader

    4) 330+
        Layout qualifiers can declare the location of vertex shader inputs and fragment shader outputs, eg:
            layout(location = 2) in vec3 values[4];
        deprecates ARB_explicit_attrib_location
        New built-ins: floatBitsToInt, floatBitsToUInt, intBitsToFloat, uintBitsToFloat

    5) 400+
        New built-ins: barrier, bitCount, bitfieldInsert, bitfieldReverse, EmitStreamVertex, EndStreamPrimitive, findLSB, findMSB, fma,
            frexp, gl_NumSamples, gl_PatchVerticesIn, gl_SampleID, gl_SampleMask, gl_SampleMaskIn, gl_SamplePosition, gl_TessCoord
            gl_TessLevelInner, gl_TessLevelOuter, umulExtended, interpolateAtCentroid, interpolateAtoOffset, interpolateAtSample
            ldexp, memoryBarrier, packDouble2x32, packUnorm2x16, packUnorm4x8, packSnorm4x8, textureGather, textureGatherOffset, textureGatherOffsets
            textureQueryLod, uaddCarrys, unpackDouble2x32, unpackUnorm2x16, unpackUnorm4x8, unpackSnorm4x8, usubBorrow

    5) 420+
        New built-ins: atomicCounterIncrement, atomicCounterDecrement, atomicCounter, imageAtomicAdd, imageAtomicCompSwap
            imageAtomicExchange, imageAtomicMax, imageAtomicMin, imageAtomicOr, imageAtomicXor, imageLoad, imageStore, packHalf2x16
            packSnorm2x16, unpackHalf2x16, unpackSnorm2x16

    6) 430+
        New built-ins: atomicXor, atomicOr, atomicMin, atomicMax, atomicExchange, atomicCompSwap, atomicAnd, gl_GlobalInvocationID,
            gl_LocalInvocationID, gl_LocalInvocationIndex, gl_NumWorkGroups, gl_WorkGroupID, gl_WorkGroupSize, groupMemoryBarrier
            imageSize, memoryBarrierAtomicCounter, memoryBarrierBuffer, memoryBarrierImage, memoryBarrierShared, textureQueryLevels
        gl_Layer in fragment shader

    7) 450+
        New built-ins: fwidthCoarse, fwidthFine, gl_CullDistance, gl_HelperInvocation, imageSamples, textureSamples
*/


VRShaderFactory::VRShaderFactory() {
    ;
}

VRShaderFactory::~VRShaderFactory() {}

void tokenize(string& text, vector<string>& tokens) {
    tokens.clear();
    char buffer[1024];
    int i = 0;
    for (char c : text) {
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r'  // any whitespace
            || c == '(') { // other delims
            buffer[i] = 0;
            if (i > 1) tokens.push_back(string(buffer));
            i = 0;
            continue;
        }

        buffer[i] = c;
        i++;
    }
}

static map<string, int> tokenVersion {
    { "trunc", 130 },
    { "round", 130 },
    { "roundEven", 130 },
    { "isnan", 130 },
    { "isinf", 130 },
    { "modf", 130 },
    { "acosh", 130 },
    { "asinh", 130 },
    { "cosh", 130 },
    { "gl_VertexID", 130 },
    { "inversesqrt", 130 },
    { "sign", 130 },
    { "sinh", 130 },
    { "smoothstep", 130 },
    { "tanh", 130 },
    { "texelFetch", 130 },
    { "texelFetchOffset", 130 },
    { "textureGrad", 130 },
    { "textureGradOffset", 130 },
    { "textureLod", 130 },
    { "textureLodOffset", 130 },
    { "textureOffset", 130 },
    { "textureProj", 130 },
    { "textureProjGrad", 1 },
    { "textureProjGradOffset", 130 },
    { "textureProjLod", 130 },
    { "textureProjLodOffset", 130 },
    { "textureProjOffset", 130 },
    { "textureSize", 130 },

    { "inverse", 140 },
    { "gl_InstanceID", 140 },

    { "determinant", 150 },
    { "EmitVertex", 150 },
    { "EndPrimitive", 150 },
    { "gl_InvocationID", 150 },
    { "gl_PrimitiveID", 150 },
    { "gl_PrimitiveIDIn", 150 },
    { "gl_ViewportIndex", 150 },
    { "gl_ClipDistance", 150 },
    { "gl_Layer in geometry shader", 150 },

    { "floatBitsToInt", 330 },
    { "floatBitsToUInt", 330 },
    { "intBitsToFloat", 330 },
    { "uintBitsToFloat", 330 },

    { "barrier", 400 },
    { "bitCount", 400 },
    { "bitfieldInsert", 400 },
    { "bitfieldReverse", 400 },
    { "EmitStreamVertex", 400 },
    { "EndStreamPrimitive", 400 },
    { "findLSB", 400 },
    { "findMSB", 400 },
    { "fma", 400 },
    { "frexp", 400 },
    { "gl_NumSamples", 400 },
    { "gl_PatchVerticesIn", 400 },
    { "gl_SampleID", 400 },
    { "gl_SampleMask", 400 },
    { "gl_SampleMaskIn", 400 },
    { "gl_SamplePosition", 400 },
    { "gl_TessCoord", 400 },
    { "gl_TessLevelInner", 400 },
    { "gl_TessLevelOuter", 400 },
    { "umulExtended", 400 },
    { "interpolateAtCentroid", 400 },
    { "interpolateAtoOffset", 400 },
    { "interpolateAtSample", 400 },
    { "ldexp", 400 },
    { "memoryBarrier", 400 },
    { "packDouble2x32", 400 },
    { "packUnorm2x16", 400 },
    { "packUnorm4x8", 400 },
    { "packSnorm4x8", 400 },
    { "textureGather", 400 },
    { "textureGatherOffset", 400 },
    { "textureGatherOffsets", 400 },
    { "textureQueryLod", 400 },
    { "uaddCarrys", 400 },
    { "unpackDouble2x32", 400 },
    { "unpackUnorm2x16", 400 },
    { "unpackUnorm4x8", 400 },
    { "unpackSnorm4x8", 400 },
    { "usubBorrow", 400 },

    { "atomicCounterIncrement", 420 },
    { "atomicCounterDecrement", 420 },
    { "atomicCounter", 420 },
    { "imageAtomicAdd", 420 },
    { "imageAtomicCompSwap", 420 },
    { "imageAtomicExchange", 420 },
    { "imageAtomicMax", 420 },
    { "imageAtomicMin", 420 },
    { "imageAtomicOr", 420 },
    { "imageAtomicXor", 420 },
    { "imageLoad", 420 },
    { "imageStore", 420 },
    { "packHalf2x16", 420 },
    { "packSnorm2x16", 420 },
    { "unpackHalf2x16", 420 },
    { "unpackSnorm2x16", 420 },

    { "atomicXor", 430 },
    { "atomicOr", 430 },
    { "atomicMin", 430 },
    { "atomicMax", 430 },
    { "atomicExchange", 430 },
    { "atomicCompSwap", 430 },
    { "atomicAnd", 430 },
    { "gl_GlobalInvocationID", 430 },
    { "gl_LocalInvocationID", 430 },
    { "gl_LocalInvocationIndex", 430 },
    { "gl_NumWorkGroups", 430 },
    { "gl_WorkGroupID", 430 },
    { "gl_WorkGroupSize", 430 },
    { "groupMemoryBarrier", 430 },
    { "imageSize", 430 },
    { "memoryBarrierAtomicCounter", 430 },
    { "memoryBarrierBuffer", 430 },
    { "memoryBarrierImage", 430 },
    { "memoryBarrierShared", 430 },
    { "textureQueryLevels", 430 },

    { "fwidthCoarse", 450 },
    { "fwidthFine", 450 },
    { "gl_CullDistance", 450 },
    { "gl_HelperInvocation", 450 },
    { "imageSamples", 450 },
    { "textureSamples", 450 }
};

int VRShaderFactory::computeGLSLVersion(string& code) {
    int version = 120;

    vector<string> tokens;
    tokenize(code, tokens);
    for (auto& token : tokens) {
        int v = tokenVersion[token];
        if (v > version) version = v;
    }

    return version;
}
