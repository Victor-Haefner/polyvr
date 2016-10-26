//----------------------------------------------------------------------------------------------MAIN--GP
#version 400 compatibility
#extension GL_EXT_geometry_shader4 : enable
layout (lines) in;
layout (triangle_strip, max_vertices = 24) out;
in vec2 tc[];

//Phong
out vec3 ViewDirection;
out vec3 fvObjectPosition;
out vec3 MVPos;
out vec3 Normal;
out vec3 TexCoord;

out float cylR1;
out float cylR2;
out vec3 cylDir;
out vec3 cylP0;
out vec3 cylP1;
out vec3 cylN0;
out vec3 cylN1;

uniform vec3 OSGCameraPosition;

#define mMVP gl_ModelViewProjectionMatrix
#define mMV gl_ModelViewMatrix
#define mP gl_ProjectionMatrix

//PHONG-----------------
void addPhongVars(vec4 p) {
   MVPos = vec3(mMV*p);
   fvObjectPosition = vec3(mMVP*p);
   ViewDirection = -fvObjectPosition.xyz;
   ViewDirection = normalize(ViewDirection);
   
   /*vec4 p1 = gl_PositionIn[0];
   vec4 p2 = gl_PositionIn[1];
   vec3 d = normalize(p2.xyz - p1.xyz);
   TexCoord = p.xyz + 0.5 * d / (1.0+10*p.xyz);*/
   
   TexCoord = p.xyz;
   TexCoord.y *= 0.15;
}

void emitTriangle(vec4 p1, vec4 p2, vec4 p3) {
   gl_Position = mMVP*p1;
   addPhongVars(p1);
   EmitVertex();
   gl_Position = mMVP*p2;
   addPhongVars(p2);
   EmitVertex();
   gl_Position = mMVP*p3;
   addPhongVars(p3);
   EmitVertex();
   EndPrimitive();
}

void emitQuad(vec4 p1, vec4 p2, vec4 p3, vec4 p4) {
   emitTriangle(p1, p2, p4);
   emitTriangle(p2, p4, p3);
}

void emitSimpleQuad(vec4 pos[4]) {
   emitQuad(pos[0], pos[1], pos[2], pos[3]);
}

float Sign(in float x) {
    return step(0, x)*2 - 1;
}

vec3 perp(vec3 v) {
    vec3 b = cross(v, vec3(1, 0, 0));
    if (dot(b, b) < 0.01) b = cross(v, vec3(0, 0, 1));
    return b;
}

void main() {
   vec4 p1 = gl_PositionIn[0];
   vec4 p2 = gl_PositionIn[1];
   
   cylR1 = tc[0][0]*0.05;
   cylR2 = tc[1][0]*0.05;
   cylDir = normalize(vec3(mMV * vec4(p2.xyz - p1.xyz,0.0)));
   //cylDir = vec3(mMV * vec4(normalize(p1.xyz - p2.xyz),0.0));
   cylP0 = vec3(mMV * p1);
   cylP1 = vec3(mMV * p2);
   //cylN0 = vec3(mMV * vec4(0.0,1.0,0.0,0.0));
   //cylN1 = vec3(mMV * vec4(0.0,1.0,0.0,0.0));
   cylN0 = cylDir;
   cylN1 = cylDir;

   vec4 pos[4];
   float S1x = 1.0;
   float S2x = -1.0;
   float S1y = 1.0;
   float S2y = -1.0;
   
   vec4 Y = p2-p1;
   vec4 X = vec4(1.0,0.0,0.0,0.0); // TODO!!!
   vec4 Z = vec4(0.0,0.0,1.0,0.0);
   //vec4 X = vec4( normalize(perp(cylDir)) ,0.0);
   //vec4 Z = vec4( normalize(cross(cylDir, X.xyz)) ,0.0);
   if ((mMVP * X).z >= 0.0) X = -X;
   if ((mMVP * Y).z >= 0.0) Y = -Y;
   if ((mMVP * Z).z >= 0.0) Z = -Z;

   pos[0] = p1+X*S1x*cylR1+Z*S1y*cylR1;
   pos[1] = p2+X*S1x*cylR2+Z*S1y*cylR2;
   pos[2] = p2+X*S2x*cylR2+Z*S1y*cylR2;
   pos[3] = p1+X*S2x*cylR1+Z*S1y*cylR1;
   emitSimpleQuad(pos);

   pos[0] = p2+X*S1x*cylR2+Z*S1y*cylR2;
   pos[1] = p1+X*S1x*cylR1+Z*S1y*cylR1;
   pos[2] = p1+X*S1x*cylR1+Z*S2y*cylR1;
   pos[3] = p2+X*S1x*cylR2+Z*S2y*cylR2;
   emitSimpleQuad(pos);
   
   vec4 pm = (p1+p2)*0.5;
   float R = cylR1;
   pos[0] = pm+X*S1x*R+Z*S1y*R+Y*0.5;
   pos[1] = pm+X*S2x*R+Z*S1y*R+Y*0.5;
   pos[2] = pm+X*S2x*R+Z*S2y*R+Y*0.5;
   pos[3] = pm+X*S1x*R+Z*S2y*R+Y*0.5;
   emitSimpleQuad(pos);
}





