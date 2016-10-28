//----------------------------------------------------------------------------------------------MAIN--GP
#version 400 compatibility
#extension GL_EXT_geometry_shader4 : enable
layout (lines) in;
layout (triangle_strip, max_vertices = 12) out;
in vec2 tc[];

//Phong
out vec3 ViewDirection;
out vec3 fvObjectPosition;
out vec3 MVPos;
out vec3 Normal;
out vec3 TexCoord;
out vec3 tmp;

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

vec4 crns[8];
vec4 crnsMV[8];
vec4 crnsMVP[8];

void emit(int i) {
	gl_Position = crnsMVP[i];
	MVPos = crnsMV[i].xyz;
	fvObjectPosition = crnsMVP[i].xyz;
	ViewDirection = -fvObjectPosition.xyz;
	ViewDirection = normalize(ViewDirection);

	/*vec4 p1 = gl_PositionIn[0];
	vec4 p2 = gl_PositionIn[1];
	vec3 d = normalize(p2.xyz - p1.xyz);
	TexCoord = p.xyz + 0.5 * d / (1.0+10*p.xyz);*/

	TexCoord = crns[i].xyz;
	TexCoord.y *= 0.15; // TODO: looks ugly!
	EmitVertex();
}

void emitQuad(int i1, int i2, int i3, int i4) {
	vec4 mvp1 = crnsMVP[i1];
	vec4 mvp2 = crnsMVP[i2];
	vec4 mvp3 = crnsMVP[i3];
	vec4 mvp4 = crnsMVP[i4];
	
	// check visibility
	vec4 c = (mvp1+mvp3)*0.5;
	vec3 i = normalize( vec3(mvp2.xyz - mvp1.xyz) );
    vec3 j = normalize( vec3(mvp4.xyz - mvp1.xyz) );
    if ( dot(cross(i, j),c.xyz) < 0.0 ) return;

	emit(i1);
	emit(i2);
	emit(i4);
	emit(i3);
	EndPrimitive();
}

vec3 perp(vec3 v) {
    vec3 b = cross(v, vec3(1, 0, 0));
    if (dot(b, b) < 0.01) b = cross(v, vec3(0, 0, 1));
    return b;
}

void main() {
   vec4 p1 = gl_PositionIn[0];
   vec4 p2 = gl_PositionIn[1];
   vec4 pm = (p1+p2)*0.5;
   vec3 dir = p2.xyz - p1.xyz;
   
   // TODO: try to make the segment bigger.
   
   cylR1 = tc[0][0]*0.1;
   cylR2 = tc[1][0]*0.1;
   cylDir = normalize(vec3(mMV * vec4(dir,0.0)));
   cylP0 = vec3(mMV * p1);
   cylP1 = vec3(mMV * p2);
   cylN0 = cylDir;
   cylN1 = cylDir;
     
   vec4 Y = vec4(dir*0.5, 0.0);
   vec4 X = vec4( normalize(perp(dir)) ,0.0);
   vec4 Z = vec4( normalize(cross(dir, X.xyz)) ,0.0);
 
   crns[0] = pm - X*cylR1 - Y - Z*cylR1;
   crns[1] = pm + X*cylR1 - Y - Z*cylR1;
   crns[2] = pm + X*cylR1 - Y + Z*cylR1;
   crns[3] = pm - X*cylR1 - Y + Z*cylR1;
   crns[4] = pm - X*cylR1 + Y - Z*cylR1;
   crns[5] = pm + X*cylR1 + Y - Z*cylR1;
   crns[6] = pm + X*cylR1 + Y + Z*cylR1;
   crns[7] = pm - X*cylR1 + Y + Z*cylR1;
   
   for (int i=0; i<8; i++) crnsMV[i] = mMV * crns[i];
   for (int i=0; i<8; i++) crnsMVP[i] = mP * crnsMV[i];
   
   emitQuad(3, 2, 1, 0); // bottom
   emitQuad(4, 5, 6, 7); // top
   emitQuad(0, 1, 5, 4); // sides
   emitQuad(1, 2, 6, 5);
   emitQuad(2, 3, 7, 6);
   emitQuad(3, 0, 4, 7);
}





