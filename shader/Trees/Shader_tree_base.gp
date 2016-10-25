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

vec3 getOffsets() {
   vec3 d = OSGCameraPosition - gl_PositionIn[1].xyz;
   return vec3(Sign(d[0]), Sign(d[1]), Sign(d[2]));
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
   cylN0 = vec3(mMV * vec4(0.0,1.0,0.0,0.0));
   cylN1 = vec3(mMV * vec4(0.0,1.0,0.0,0.0));
   
   gl_FrontColor = vec4(0.6, 0.6, 0.3, 1.0);

   vec3 offs = getOffsets();
   vec4 pos[4];
   float S1x = 0.05*offs.x;
   float S2x = -0.05*offs.x;
   float S1y = 0.05*offs.z;
   float S2y = -0.05*offs.z;

   pos[0] = p1+vec4(S1x,0,S1y,0)*tc[0][0];
   pos[1] = p2+vec4(S1x,0,S1y,0)*tc[1][0];
   pos[2] = p2+vec4(S2x,0,S1y,0)*tc[1][0];
   pos[3] = p1+vec4(S2x,0,S1y,0)*tc[0][0];
   emitSimpleQuad(pos);

   pos[0] = p2+vec4(S1x,0,S1y,0)*tc[1][0];
   pos[1] = p1+vec4(S1x,0,S1y,0)*tc[0][0];
   pos[2] = p1+vec4(S1x,0,S2y,0)*tc[0][0];
   pos[3] = p2+vec4(S1x,0,S2y,0)*tc[1][0];
   emitSimpleQuad(pos);
   
   vec4 pc = p1;
   float R = tc[0][0];
   if (offs.y > 0) { pc = p2; R = tc[1][0]; }
   pos[0] = pc+vec4(S1x,0,S1y,0)*R;
   pos[1] = pc+vec4(S2x,0,S1y,0)*R;
   pos[2] = pc+vec4(S2x,0,S2y,0)*R;
   pos[3] = pc+vec4(S1x,0,S2y,0)*R;
   emitSimpleQuad(pos);
}





