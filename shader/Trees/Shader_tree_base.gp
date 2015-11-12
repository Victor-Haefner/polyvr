#version 150
#extension GL_EXT_geometry_shader4 : enable
layout (lines) in;
layout (triangle_strip, max_vertices = 16) out;
in mat4 model[];
in vec2 tc[];
in mat3 n_model[];
in vec3 vert_normal[];

//Phong
out vec3 fvObjectPosition;
out vec3 Normal;
out vec2 texCoord;

void emitQuad(vec4 p1, vec4 p2, vec4 p3, vec4 p4, vec2 tc1, vec2 tc2, vec2 tc3, vec2 tc4) {
   gl_Position = p1;
   fvObjectPosition = vec3(model[0] * p1);
   texCoord = tc1;
   EmitVertex();

   gl_Position = p2;
   fvObjectPosition = vec3(model[0] * p2);
   texCoord = tc2;
   EmitVertex();

   gl_Position = p4;
   fvObjectPosition = vec3(model[0] * p4);
   texCoord = tc4;
   EmitVertex();

   gl_Position = p3;
   fvObjectPosition = vec3(model[0] * p3);
   texCoord = tc3;
   EmitVertex();

   EndPrimitive();
}

void emitSimpleQuad(vec4 pos[4]) {
   Normal = n_model[0] * cross(vec3(pos[1]-pos[0]),vec3(pos[1]-pos[2]));
   for (int i=0;i<4;i++) pos[i] = model[0]*pos[i];
   emitQuad(pos[0], pos[1], pos[2], pos[3], vec2(0,0), vec2(1,0), vec2(1,1), vec2(0,1));
}

void main() {
   //vec4 p1 = model[0]*gl_PositionIn[0];
   //vec4 p2 = model[0]*gl_PositionIn[1];
   vec4 p1 = gl_PositionIn[0];
   vec4 p2 = gl_PositionIn[1];
   gl_FrontColor = vec4(0.6, 0.6, 0.3, 1.0);

   vec4 pos[4];

   pos[0] = p1;
   pos[1] = p2;
   pos[2] = p2+vec4(0.1,0,0,0)*tc[1][0];
   pos[3] = p1+vec4(0.1,0,0,0)*tc[0][0];
   emitSimpleQuad(pos);

   pos[0] = p2;
   pos[1] = p1;
   pos[2] = p1+vec4(0,0,0.1,0)*tc[0][0];
   pos[3] = p2+vec4(0,0,0.1,0)*tc[1][0];
   emitSimpleQuad(pos);

   pos[0] = p1+vec4(0.1,0,0,0)*tc[0][0];
   pos[1] = p2+vec4(0.1,0,0,0)*tc[1][0];
   pos[2] = p2+vec4(0.1,0,0.1,0)*tc[1][0];
   pos[3] = p1+vec4(0.1,0,0.1,0)*tc[0][0];
   emitSimpleQuad(pos);

   pos[0] = p2+vec4(0,0,0.1,0)*tc[1][0];
   pos[1] = p1+vec4(0,0,0.1,0)*tc[0][0];
   pos[2] = p1+vec4(0.1,0,0.1,0)*tc[0][0];
   pos[3] = p2+vec4(0.1,0,0.1,0)*tc[1][0];
   emitSimpleQuad(pos);
}
