//----------------------------------------------------------------------------------------------MAIN--GP
#version 400 compatibility
#extension GL_EXT_geometry_shader4 : enable
layout (points) in;
layout (triangle_strip, max_vertices = 6) out;
in vec3 vn[];
in vec3 vcol[];
out vec2 tcs;
out vec3 norm;
out vec3 col;
out vec4 position;

#define mMVP gl_ModelViewProjectionMatrix
#define mMV gl_ModelViewMatrix
#define mP gl_ProjectionMatrix

vec4 crns[4];
vec4 crnsMV[4];
vec4 crnsMVP[4];
vec2 TCS[4];

void emit(int i) {
	gl_Position = crnsMVP[i];
	position = crnsMV[i];
	tcs = TCS[i];
	EmitVertex();
}

void main() {
	vec4 p = gl_PositionIn[0];
	float s = vcol[0].r;
	vec3 u = vec3(0,1,0);
	//vec4 x = vec4(s,0,0,0);
	//vec4 z = vec4(0,0,s,0);
	vec4 z = vec4(vn[0]*s, 0);
	vec4 x = vec4(normalize(cross(z.xyz,u))*s, 0);
	if (z[1] > 0) z[1] *= -1;
	crns[0] = p -x-z;
	crns[1] = p +x-z;
	crns[2] = p -x+z;
	crns[3] = p +x+z;
	TCS[0] = vec2(0,0);
	TCS[1] = vec2(1,0);
	TCS[2] = vec2(0,1);
	TCS[3] = vec2(1,1);
	norm = normalize(cross(x.xyz,z.xyz));
	col = vcol[0];
	for (int i=0; i<4; i++) crnsMV[i] = mMV * crns[i];
	for (int i=0; i<4; i++) crnsMVP[i] = mMVP * crns[i];
	emit(0);
	emit(1);
	emit(2);
	emit(3);
	EndPrimitive();
}





