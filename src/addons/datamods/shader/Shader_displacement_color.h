#ifndef SHADER_DISPLACEMENT_COLOR_H_INCLUDED
#define SHADER_DISPLACEMENT_COLOR_H_INCLUDED

#include <OpenSG/OSGSimpleSHLChunk.h>
#include <OpenSG/OSGMaterialGroup.h>

using namespace std;

OSG_BEGIN_NAMESPACE;

class dcShader {
    private:
        SimpleSHLChunkRecPtr shader_chunk;

        //----------------------------------------------------
        //phong Shader
        static string vs_program;
        static string fs_program;

    public:
        dcShader(ChunkMaterialRecPtr mat, float Dtc) {
            shader_chunk = SimpleSHLChunk::create();

            shader_chunk->setVertexProgram(vs_program);
            shader_chunk->setFragmentProgram(fs_program);

            float dtc = 1./Dtc;
            shader_chunk->addUniformVariable("dtc", dtc);
            shader_chunk->addUniformVariable("Dtc", Dtc);
            shader_chunk->addUniformVariable("displacementMap", 0);

            //shader_chunk->updateUniformVariable("dtc", dtc);
            //shader_chunk->updateUniformVariable("Dtc", Dtc);
            //shader_chunk->updateUniformVariable("displacementMap", 0);


            mat->addChunk(shader_chunk);
        }
};

// vertex shader program (phong)
string dcShader::vs_program = //color shader ist jetzt hier dabei!!
"uniform sampler2D displacementMap;\n"
"uniform float dtc;\n"//texel size
"uniform float Dtc;\n"//texture size

"vec4 c_p0  = vec4(0.0, 0.0, 0.5, 1.0);\n"
"vec4 c_p1  = vec4(1.0,  1.0,  0.0,  1.0);\n"
"vec4 c_p2  = vec4(0.0,  1.0,  0.0,  1.0);\n"
"vec4 c_p3  = vec4(1.0,  0.0,  0.0,  1.0);\n"
"varying vec3 ViewDirection;\n"
"varying vec3 fvObjectPosition;\n"
"varying vec3 Normal;\n"
"float s;\n"//scale

//for debugging
"varying vec2 tc;\n"

//---------------------------------------------------------------------------------texture2D_bilinear
"vec4 texture2D_bilinear( sampler2D tex, vec2 uv ) {\n"
"	vec2 f = fract( uv.xy * Dtc );\n"
"	vec4 t00 = texture2D( tex, uv );\n"
"	vec4 t10 = texture2D( tex, uv + vec2( dtc, 0.0 ));\n"
"	vec4 tA = mix( t00, t10, f.x );\n"
"	vec4 t01 = texture2D( tex, uv + vec2( 0.0, dtc ) );\n"
"	vec4 t11 = texture2D( tex, uv + vec2( dtc, dtc ) );\n"
"	vec4 tB = mix( t01, t11, f.x );\n"
"	return mix( tA, tB, f.y );\n"
"}\n"

//---------------------------------------------------------------------------------DISPLACEMENT FKT
"vec4 getDisplacementFromMap( vec2 tc ) {\n"
"    vec4 v;\n"
"    vec4 c;\n"
"    float df;\n"//displacement
//"    c = texture2D_bilinear(displacementMap, tc * dtc);\n"
"    c = texture2D(displacementMap, tc * dtc);\n"
"    s = 1000.0*c[3];\n"
"    df = c.x*s + c.y*s*0.00392156862745 + c.z*s*1.53787004998e-05;\n"
"    v = vec4(0.0, df, 0.0, 0.0);\n"
"    return v;\n"
"}\n"

//----------------------------------------------------------------------------------------------MAIN--VP
"void main( void ) {\n"
//displacement shader-----------------

"    tc = gl_MultiTexCoord0.xy;\n"
"    vec4 disp0 = getDisplacementFromMap(tc);\n"
"    vec4 new_v_pos = disp0 + gl_Vertex;\n"

"    gl_Position = gl_ModelViewProjectionMatrix * new_v_pos;\n"

//NORMALS----------------------
"    "
"    vec4 dispx1 = vec4(0.0, 0.0, 0.0, 0.0);\n"
"    vec4 dispx2 = dispx1;\n"
"    vec4 dispy1 = dispx1;\n"
"    vec4 dispy2 = dispx1;\n"

"    dispx1 = getDisplacementFromMap(tc + vec2(1.0, 0.0));\n"
"    dispx2 = getDisplacementFromMap(tc + vec2(-1.0, 0.0));\n"
"    dispy1 = getDisplacementFromMap(tc + vec2(0.0, 1.0));\n"
"    dispy2 = getDisplacementFromMap(tc + vec2(0.0, -1.0));\n"

"    vec3 tanx1 = vec3(dispx1 - disp0) + vec3(1.0, 0.0, 0.0);\n"
"    vec3 tanx2 = vec3(dispx2 - disp0) + vec3(-1.0, 0.0, 0.0);\n"
"    vec3 tany1 = vec3(dispy1 - disp0) + vec3(0.0, 0.0, 1.0);\n"
"    vec3 tany2 = vec3(dispy2 - disp0) + vec3(0.0, 0.0, -1.0);\n"
"    vec3 normal1 = cross(tany1, tanx1);\n"
"    vec3 normal2 = cross(tany2, tanx2);\n"
"    vec3 normal3 = cross(tanx2, tany1);\n"
"    vec3 normal4 = cross(tanx1, tany2);\n"
"    vec3 normal = normal1*0.5 + normal2*0.5 + normal3*0.5 + normal4*0.5;\n"
//"    vec3 normal = gl_Normal;\n"

// PHONG-----------------
"    fvObjectPosition = vec3(gl_ModelViewMatrix * new_v_pos);\n"
"    ViewDirection   = - fvObjectPosition.xyz;\n"
"    Normal          = gl_NormalMatrix * normal;\n"
//"   Normal         = gl_NormalMatrix * gl_Normal;\n"

//color---------------------
"    float t = 32.0*new_v_pos.y/s;\n"
//"    gl_FrontColor       = t*t*t*(-c_p0 + 3.0*c_p1 - 3.0*c_p2 + c_p3) + t*t*3.0*(c_p0 - 2.0*c_p1 + c_p2) + t*3.0*(c_p1 - c_p0) + c_p0;\n" //define the base color depending on vertex height using bezier algo
"    gl_FrontColor       = c_p0 + t*c_p1 - t*c_p0;\n" //define the base color depending on vertex height using bezier algo

"}\n";


//----------------------------------------------------------------------------------------------MAIN--FP
// fragment shader program for bump mapping in surface local coordinates (phong)
string dcShader::fs_program =
"uniform sampler2D displacementMap;\n"
"uniform float dtc;\n"//texel size
"vec4 fvAmbient  = vec4(0.36, 0.36, 0.36, 1.0);\n"
"vec4 fvSpecular = vec4(0.7,  0.7,  0.7,  1.0);\n"
"vec4 fvDiffuse  = vec4(0.5,  0.5,  0.5,  1.0);\n"
"float fSpecularPower = 25.0;\n"
"\n"
"uniform sampler2D baseMap;\n"
"uniform int useTexture;\n"
"\n"
"varying vec2 tc;\n"
"varying vec3 ViewDirection;\n"
"varying vec3 fvObjectPosition;\n"
"varying vec3 Normal;\n"
"\n"
"void main( void )\n"
"{\n"
"   vec3  fvLightDirection = normalize( gl_LightSource[0].position.xyz - fvObjectPosition.xyz);\n"
"   vec3  fvNormal         = normalize( Normal );\n"
"   float fNDotL           = dot( fvNormal, fvLightDirection ); \n"
"   \n"
"   vec3  fvReflection     = normalize( ( ( 2.0 * fvNormal ) * fNDotL ) - fvLightDirection ); \n"
"   vec3  fvViewDirection  = normalize( ViewDirection );\n"
"   float fRDotV           = max( 0.0, dot( fvReflection, fvViewDirection ) );\n"
"   \n"
"   vec4  fvBaseColor      = gl_Color;\n"
"   \n"
"   vec4  fvTotalAmbient   = fvAmbient * fvBaseColor; \n"
"   vec4  fvTotalDiffuse   = fvDiffuse * fNDotL * fvBaseColor; \n"
"   vec4  fvTotalSpecular  = fvSpecular * ( pow( fRDotV, fSpecularPower ) );\n"
"   \n"
"   gl_FragColor = ( fvTotalAmbient + fvTotalDiffuse + fvTotalSpecular );\n"
"   gl_FragColor = gl_Color;\n"
//"   gl_FragColor = texture2D(displacementMap, tc*dtc);\n"
"}\n";

OSG_END_NAMESPACE;

#endif // SHADER_DISPLACEMENT_COLOR_H_INCLUDED
