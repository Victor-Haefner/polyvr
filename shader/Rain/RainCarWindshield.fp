#version 400 compatibility
#define M_PI 3.1415926535897932384626433832795

// gen
in vec3 norm;
in vec4 pos;
in vec2 tcs;
in mat3 miN;
in mat4 miP;
vec3 fragDir;
vec3 pWindshield; //point in plane of windshield
vec4 color;
bool debugB = false;

uniform vec2 OSGViewportSize;
uniform float tnow;
uniform float offset;
uniform float rainDensity;
uniform vec3 carOrigin;
uniform vec3 carDir;
uniform vec3 posOffset;

void computeDirection() {
	fragDir = normalize( miN * (miP * pos).xyz );
}

float hash(vec2 co) {
    	return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

float gettheta(vec3 d) {
	return acos(d.y);
}

void computeDepth(vec4 position) {
	float d = position.z / position.w;
	gl_FragDepth = d*0.5 + 0.5;
}

float computeDropSize() {
	float dropsize = 1;
	dropsize = 0.99998;
	return dropsize;
}

vec4 drawDot(vec3 P0, vec4 check) {
	mat4 m = inverse(gl_ModelViewMatrix);
	vec3 PCam = (m*vec4(0,0,0,1)).xyz;
	vec3 D0 = normalize( P0-PCam );

	if (mod(tnow,1)<0.02) {
		computeDepth(gl_ModelViewProjectionMatrix*vec4(P0,1));
		if (dot(D0,fragDir) > computeDropSize()) check = vec4(0,0,1,1);
	}
	if (mod(tnow,1)>0.02 && mod(tnow,1)<0.2) {
		computeDepth(gl_ModelViewProjectionMatrix*vec4(P0,1));
		if (dot(D0,fragDir) > computeDropSize()) check = vec4(0,0,1,0.2);
	}
	return check;
}

vec4 drawCenter(vec3 P0, vec4 check) {
	mat4 m = inverse(gl_ModelViewMatrix);
	vec3 PCam = (m*vec4(0,0,0,1)).xyz;	
	vec3 D0 = normalize( P0-PCam );

	if (dot(D0,fragDir) > 0.9994) { 
		check = vec4(1,0,0,1);
		computeDepth(gl_ModelViewProjectionMatrix*vec4(P0,1));
	}
	return check;
}

void main() {
	computeDirection();
	vec3 posOffsets = vec3(0.3,0.7,0);
	pWindshield = carOrigin + posOffsets;
	
	if (fragDir.y < -0.999) discard;
	vec4 check = vec4(0,0,0,0);	

	check = drawDot(pWindshield, check);
	check = drawDot(pWindshield + vec3(0,0,0.2),check);
	check = drawDot(pWindshield + vec3(0,0,0.4),check);
	check = drawDot(pWindshield + vec3(0,0,0.6),check);
	check = drawDot(pWindshield + vec3(0,0,0.8),check);
	check = drawDot(pWindshield + vec3(0,0,1),check);
	check = drawDot(pWindshield + vec3(0,0.2,0),check);
	check = drawDot(pWindshield + vec3(0,0.2,0.2),check);
	check = drawDot(pWindshield + vec3(0,0.2,0.4),check);
	check = drawDot(pWindshield + vec3(0,0.2,0.6),check);
	check = drawDot(pWindshield + vec3(0,0.2,0.8),check);
	check = drawDot(pWindshield + vec3(0,0.2,1),check);
	check = drawCenter(vec3(0,0,0), check);
	
	//computeDepth(gl_ModelViewProjectionMatrix*vec4(P0,1));
	if (check == vec4(0,0,0,0)) discard;	
	gl_FragColor = check;
}





