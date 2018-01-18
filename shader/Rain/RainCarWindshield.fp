#version 400 compatibility
#define M_PI 3.1415926535897932384626433832795

// gen
in vec3 norm;
in vec4 pos;
in vec2 tcs;
in mat3 miN;
in mat4 miP;
vec3 fragDir;
vec3 PCam;
vec3 axU;
vec3 axV;
vec4 color;
bool debugB = false;

uniform vec2 OSGViewportSize;
uniform float tnow;
uniform float offset;
uniform float rainDensity;
uniform vec3 carOrigin;
uniform vec3 carDir;
uniform vec3 posOffset;

uniform vec3 windshieldPos;
uniform vec3 windshieldDir;
uniform vec3 windshieldUp;

void computeDirection() {
	fragDir = normalize( miN * (miP * pos).xyz );
}

float hash(vec2 co) {
    	return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

float gettheta(vec3 d) {
	return acos(d.y);
}

void computePCam() {
	mat4 m = inverse(gl_ModelViewMatrix);
	PCam = (m*vec4(0,0,0,1)).xyz;
}

void computeDepth(vec4 position) {
	float d = position.z / position.w;
	gl_FragDepth = d*0.5 + 0.5;
}

float computeDropSize() {
	float dropsize = 1;
	//dropsize = 0.99998;
	dropsize = 0.9998;
	return dropsize;
}

vec3 computeDropOnWS() {
	float x = dot((windshieldPos - PCam),windshieldUp)/(dot(fragDir,windshieldUp));
	vec3 p = x * fragDir + PCam;
	computeDepth(gl_ModelViewProjectionMatrix*vec4(p,1));
	return p;
}

vec4 drawDot(vec3 P0, vec4 check) {
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

vec3 localToWorld(vec2 inVec) {
	vec3 outVec;
	outVec = windshieldPos + axU*inVec.x + axV*inVec.y;
	return outVec;
}

vec2 worldToLocal(vec3 inV) {
	vec2 outV;
	outV.x = dot((inV-windshieldPos),axU);
	outV.y = dot((inV-windshieldPos),axV);
	return outV;
}

vec4 drawCenter(vec3 P0, vec4 check) {	
	vec3 D0 = normalize( P0-PCam );

	if (dot(D0,fragDir) > 0.9994) { 
		check = vec4(1,0,0,1);
		computeDepth(gl_ModelViewProjectionMatrix*vec4(P0,1));
	}
	return check;
}

void main() {
	computeDirection();
	computePCam();
	axU = cross(windshieldDir,windshieldUp);
	axV = windshieldDir;
	
	if (fragDir.y < -0.999) discard;
	vec4 check = vec4(0,0,0,0);	
	/*
	check = drawDot(windshieldPos, check);
	check = drawDot(windshieldPos + windshieldDir*0.2,check);
	check = drawDot(windshieldPos + windshieldDir*0.4,check);
	check = drawDot(windshieldPos + windshieldDir*(-0.2),check);
	check = drawDot(windshieldPos + windshieldDir*(-0.4),check);

	check = drawDot(windshieldPos + windshieldUp*0.2,check);
	check = drawDot(windshieldPos + windshieldUp*0.4,check);
	check = drawDot(windshieldPos + windshieldUp*(-0.2),check);
	check = drawDot(windshieldPos + windshieldUp*(-0.4),check);
	check = drawCenter(vec3(0,0,0), check);
	*/
	//if(distance(windshieldPos,computeDropOnWS()) < 1.5) check = vec4(1,0,0,0.8);
	check = drawDot(localToWorld(vec2(0,0)),check);
	check = drawDot(localToWorld(vec2(0,0.5)),check);
	check = drawDot(localToWorld(vec2(1,0)),check);
	check = drawDot(localToWorld(vec2(1,0.5)),check);

	

	//computeDepth(gl_ModelViewProjectionMatrix*vec4(P0,1));
	if (check == vec4(0,0,0,0)) discard;	
	gl_FragColor = check;
}





