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
uniform vec2 OSGViewportSize;

float theta;

void computeDirection() {
	fragDir = normalize( miN * (miP * pos).xyz );
}

// returns the x-z coordinates on plane y = h
// at which the ray fragDir intersects
vec2 planeIntersect(float h){
	vec2 pt = vec2(0.);
	// r = h / cos \phi
	pt = - h * fragDir.xz / fragDir.y ;
	return pt;
}

void main() {
	computeDirection();

	// \theta is angle of fragDir to zenith
	theta = acos(fragDir.y);

	// vertical line for testing
	//if (fragDir.x < 0.01 && fragDir.x > 0) gl_FragColor = vec4(0,0,0,1);
	//gl_FragColor = vec4(fragDir.yxz, 1.0);

	//if (theta < 0.5*M_PI) discard;
	//if (fragDir.y < 0) discard;

	mat4 m = inverse(gl_ModelViewMatrix);
	vec3 PCam = (m*vec4(0,0,0,1)).xyz;
	vec3 P0 = vec3(0,100,0);
	vec3 D0 = normalize( P0-PCam );
	//if (dot(D0,fragDir) < 0.9999 && dot(D0,fragDir) > 0.9) discard;

	vec3 P1 = vec3(20,110,0);
	vec3 D1 = normalize( P1-PCam );
	if (dot(D1,fragDir) < 0.9999 && dot(D1,fragDir) > 0.9) discard;
	//if (dot(D1,fragDir) > 0.99999) discard;
	//if (dot(D0,fragDir) > 0.99999) discard;
	//discard;
	//if (dot(D1,fragDir) < 0.9999) gl_FragColor = vec4(0,0,0.5, 1.0);

	gl_FragColor = vec4(0,0,0.5, 1.0);
	//gl_FragColor = vec4(-fragDir, 1.0);
}





