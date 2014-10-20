//----------------------------------------------------------------------------------------------MAIN--GP
#version 150
#extension GL_EXT_geometry_shader4 : enable
layout (points) in;
layout (triangle_strip, max_vertices = 12) out;

//vp input
in mat4 model[];
in vec4 light1[];
in vec4 light2[];

//fp output
out vec2 texCoord;
out float light;

vec4 po[8];
int i;
vec4 lights[2];

uniform vec4 cam_pos;
uniform vec4 Vox0;
uniform vec4 Vox1;
uniform vec4 Vox2;

uniform vec2 tc1;
uniform vec2 tc2;
uniform vec2 tc3;
uniform vec2 tc4;
uniform vec2 tc5 = vec2(0.5,0.5);

void emitVertex(in vec4 p, in vec2 tc, in float l) {
   gl_Position = p;
   texCoord = tc;
   light = l;
   EmitVertex();
}

void emitQuad(in vec4 p1, in vec4 p2, in vec4 p3, in vec4 p4, in float l1, in float l2, in float l3, in float l4) {
   vec4 p5  = (p1+p2+p3+p4)*0.25;
   float l5 = (l1+l2+l3+l4)*0.25;

   emitVertex(p1, tc1, l1);
   emitVertex(p2, tc2, l2);
   emitVertex(p5, tc5, l5);
   EndPrimitive();

   emitVertex(p2, tc2, l2);
   emitVertex(p3, tc3, l3);
   emitVertex(p5, tc5, l5);
   EndPrimitive();

   emitVertex(p3, tc3, l3);
   emitVertex(p4, tc4, l4);
   emitVertex(p5, tc5, l5);
   EndPrimitive();

   emitVertex(p4, tc4, l4);
   emitVertex(p1, tc1, l1);
   emitVertex(p5, tc5, l5);
   EndPrimitive();
}

//returns -1.0 if x < 0, and 1.0 if x >= 0
float Sign(in float x) {
    return step(0, x)*2 - 1;
}

void main() {
   lights[0] = light1[0];
   lights[1] = light2[0];

   int sid[3];
   sid[0] = gl_PrimitiveIDIn%3;
   sid[1] = (sid[0]+1)%3;
   sid[2] = (sid[0]+2)%3;

   vec4 d = cam_pos - gl_PositionIn[0];
   float sd = Sign(d[sid[0]]);
   int ls = int(step(0, d[sid[0]]));

   vec4 mp = model[0]*gl_PositionIn[0];

   vec4 mv[3];
   mv[0] = 0.5*model[0]*Vox0;
   mv[1] = 0.5*model[0]*Vox1;
   mv[2] = 0.5*model[0]*Vox2;

   mp += sd*mv[sid[0]];

   vec4 p[4];
   p[0] = mp - mv[sid[1]] - sd*mv[sid[2]];
   p[1] = mp + mv[sid[1]] - sd*mv[sid[2]];
   p[2] = mp + mv[sid[1]] + sd*mv[sid[2]];
   p[3] = mp - mv[sid[1]] + sd*mv[sid[2]];

   emitQuad(p[0], p[1], p[2], p[3], lights[ls][0], lights[ls][1], lights[ls][3], lights[ls][2]);
}
