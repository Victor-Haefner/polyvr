#version 400 compatibility
#define M_PI 3.1415926535897932384626433832795

// gen
in vec3 norm;
in vec4 pos;
in vec2 tcs;
in mat3 miN;
in mat4 miP;
vec3 fragDir;
vec4 color;
bool debugB = false;

uniform vec2 OSGViewportSize;
uniform float rainOffset;
uniform float rainDensity;

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

void ccDepth(float D) {
	//float dd = (D/(256-0.1));
	gl_FragDepth = 0.998;//dd*0.5 + 0.5;
}

void main() {
	computeDirection();

	// \theta is angle of fragDir to vertical axis
	//theta = acos(fragDir.y);

	mat4 m = inverse(gl_ModelViewMatrix);
	vec3 PCam = (m*vec4(0,0,0,1)).xyz;
	vec3 P0 = vec3(0,10,0);
	vec3 T0 = P0-PCam;
	vec3 D0 = normalize( P0-PCam );
	//if (dot(D0,fragDir) < 0.9999 && dot(D0,fragDir) > 0.999) discard;
	if (dot(D0,fragDir) < 0.9999 && dot(D0,fragDir) > -10) discard;

	vec3 check = vec3(1,0,0);
	ccDepth(256);	
	//discard;
	computeDepth(gl_ModelViewProjectionMatrix*vec4(P0,1));
	gl_FragColor = gl_FragColor + vec4(check,0.6);
}





