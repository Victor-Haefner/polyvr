#ifndef NEUTRON_FLUX_SHADER_H_INCLUDED
#define NEUTRON_FLUX_SHADER_H_INCLUDED

#include <OpenSG/OSGSimpleSHLChunk.h>
#include <OpenSG/OSGMaterialGroup.h>

using namespace std;

OSG_BEGIN_NAMESPACE;

class neutron_flux_shader {
    private:
        SimpleSHLChunkRecPtr shader_chunk;

        //----------------------------------------------------
        //phong Shader
        static string vs_program;
        static string fs_program;
        static string gs_program;

        float scale;

    public:
        neutron_flux_shader(ChunkMaterialRecPtr mat, int W, int H) {
            shader_chunk = SimpleSHLChunk::create();

            shader_chunk->setVertexProgram(vs_program);
            shader_chunk->setFragmentProgram(fs_program);
            shader_chunk->setGeometryProgram(gs_program);

            float w, h; w = 1./W; h = 1./H;
            scale = 1;
            shader_chunk->addUniformVariable("W", w);
            shader_chunk->addUniformVariable("H", h);
            shader_chunk->addUniformVariable("scale", scale);
            shader_chunk->addUniformVariable("displacementMap", 0);

            mat->addChunk(shader_chunk);
        }

        void setScale(float s) {shader_chunk->updateUniformVariable("scale", s); scale = s;}

        void multScale(float s) {scale *= s; setScale(scale);}
};

// vertex shader program
string neutron_flux_shader::vs_program =
"uniform sampler2D displacementMap;\n"
"uniform float W;\n"//texel size
"uniform float H;\n"//texel size
"uniform float scale;\n"//texture size
"float s;\n"//scale

//---------------------------------------------------------------------------------GET DATA FROM TEXTURE
"float getDataFromMap( vec2 tc ) {\n"
"    vec4 c;\n"
"    float df;\n"//float
"    tc[0] *= W;\n"
"    tc[1] *= H;\n"
"    c = texture2D(displacementMap, tc);\n"
"    df = c.x + c.y*0.00392156862745 + c.z*1.53787004998e-05;\n"
"    return df;\n"
"}\n"

//----------------------------------------------------------------------------------------------MAIN--VP
"void main( void ) {\n"

"    vec2 tc = gl_MultiTexCoord0.xy;\n"
"    float val = getDataFromMap(tc)*scale;\n"
"    gl_Position = gl_ModelViewProjectionMatrix*gl_Vertex;\n"
"    gl_FrontColor = vec4(1.0, 0.0, 0.0, val);\n"
//"    gl_FrontColor = vec4(1.0, 0.0, 0.0, 0.6);\n"
//"    gl_FrontColor = vec4(1.0, 1.0 - val, 0.0, 0.05);\n"//geht gut :)

"}\n";

//#extension GL_ARB_geometry_shader4 : enable

//----------------------------------------------------------------------------------------------MAIN--GP
string neutron_flux_shader::gs_program =
"#version 150\n"
"#extension GL_EXT_geometry_shader4 : enable\n"
"layout (points) in;\n"
"layout (triangle_strip, max_vertices = 6) out;\n"
"void main()\n"
"{\n"
"   gl_FrontColor = gl_FrontColorIn[0];\n"
"   \n"
"   gl_Position = gl_PositionIn[0];\n"
"   \n"
"   gl_Position.x -= 0.0085;\n"
"   gl_Position.y -= 0.0085;\n"
"   EmitVertex();\n"
"   \n"
"   gl_Position.x += 0.017;\n"
"   EmitVertex();\n"
"   \n"
"   gl_Position.y += 0.017;\n"
"   EmitVertex();\n"
"   \n"
"   EndPrimitive();\n"
"   \n"
"   EmitVertex();\n"
"   \n"
"   gl_Position.x -= 0.017;\n"
"   EmitVertex();\n"
"   \n"
"   gl_Position.y -= 0.017;\n"
"   EmitVertex();\n"
"   \n"
"   EndPrimitive();\n"
"}\n";


//----------------------------------------------------------------------------------------------MAIN--FP
string neutron_flux_shader::fs_program =
"void main( void )\n"
"{\n"
"   gl_FragColor = gl_Color;\n"
"}\n";

OSG_END_NAMESPACE;

#endif // NEUTRON_FLUX_SHADER_H_INCLUDED
