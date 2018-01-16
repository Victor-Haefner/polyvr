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

void main() {
	computeDirection();
	vec3 posOffsets = vec3(0.3,0.7,0);
	pWindshield = carOrigin + posOffsets;
	
	if (fragDir.y < -0.999) discard;

	// \theta is angle of fragDir to vertical axis
	//theta = acos(fragDir.y);

	mat4 m = inverse(gl_ModelViewMatrix);
	vec3 PCam = (m*vec4(0,0,0,1)).xyz;
	//vec3 P0 = vec3(-10,1,0);
	
	vec3 P0 = pWindshield;
	//vec3 T0 = P0-PCam;
	vec3 D0 = normalize( P0-PCam );
	//if (dot(D0,fragDir) < 0.9999 && dot(D0,fragDir) > 0.999) discard;
	computeDepth(gl_ModelViewProjectionMatrix*vec4(P0,1));	
	
	
	
	vec4 check = vec4(0.1,0.1,0.1,0.4);
	//gl_FragDepth = 0.9;
	if (mod(tnow,1)<0.02) {
		if (dot(D0,fragDir) > 0.999) check = vec4(0,0,1,1);
	}
	if (mod(tnow,1)>0.02 && mod(tnow,1)<0.2) {
		if (dot(D0,fragDir) > 0.999) check = vec4(0,0,1,0.2);
	}

	vec3 Po0 = vec3(0,0,0);
	vec3 Do0 = normalize( Po0-PCam );
	if (dot(Do0,fragDir) > 0.999) { 
		check = vec4(1,0,0,1);
		computeDepth(gl_ModelViewProjectionMatrix*vec4(Po0,1));
	}

	//computeDepth(gl_ModelViewProjectionMatrix*vec4(P0,1));
	gl_FragColor = check;
}





