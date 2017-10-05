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
uniform float rainOffset;

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

float getphi(in float z, in float x, in float y){
	//Kugelkoords, x=y, y=z, z=x
	if (z>0) return atan(x/z);
	if (z==0) return sign(x)*M_PI/2;
	if (z<0 && x>=0) return atan(x/z)+M_PI;
	if (z<0 && x<0) return atan(x/z)-M_PI;
}
float gettheta(in float z, in float x, in float y){
	return acos(y/sqrt(z*z + x*x + y*y));
}
float getdropsize(in float theta){
	if (theta < M_PI/2){
		return 0.2*2*theta/M_PI;
	} else {
		if (theta > M_PI/2){
			return 0.2*2*(M_PI-theta)/M_PI;
		} else return 0;
	}
}
float getOffset(in float rOffset, in float dropdis) {
	return mod(rOffset,dropdis);
}

vec3 checkrad(in float x, in float y, in float z)
{
  	float dropsize = getdropsize(gettheta(z,x,y));
	float dropdis = 10;
	float dropdisy = 45;
	float toffset = rainOffset;  	


	float israindrop = mod(getphi(z,x,y)*180/M_PI,dropdis);
	//float israindropm = mod(getphi(z,-x,-y)*180/M_PI,dropdis);  // + mod(asin(z)*180/M_PI,10.0);
	float israindroph = mod(gettheta(z,x,y)*180/M_PI-getOffset(toffset,dropdisy),dropdisy);
	//float israindrophm = mod(gettheta(z,-x,y)*180/M_PI-getOffset(toffset,dropdis),dropdis);

	if (israindrop < dropsize && israindroph < 5*dropsize) {
		//|| (israindropm < dropsize && israindrophm < 5*dropsize)){			
		return vec3(0,0,0.8);
	} else {
		discard;
		
	}
}

vec3 checkpoint(in float x, in float y, in float z)
{
  	float dropsize = 0.5;
	float dropdis = 10;	//later as uniform?!, make variable as how strong rain is
  	
	//TODO: write noise function, to shift initial position of raindrops
	//x-xnoise, y-ynoise, z-ynoise

	//TODO: update y-value, to shift position of raindrops due to time - simulate falling
	//y-yshift
	
	//use next line if specific 3D-point for location of texture
	//if( mod(x,dropdis)==0 && mod(y,dropdis)==0 && mod(z,dropdis)==0 ) {

	if( mod(x,dropdis)<dropsize && mod(y,dropdis)<dropsize && mod(z,dropdis)<dropsize ) {
		return vec3(0,0,0.8);
	} else {
		discard;
	}
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
	vec3 T0 = P0-PCam;
	vec3 D0 = normalize( P0-PCam );
	//if (dot(D0,fragDir) < 0.9999 && dot(D0,fragDir) > 0.999) discard;

	//vec3 check = checkrad(fragDir.x, fragDir.y, fragDir.z, D0, T0);
	vec3 check = checkrad(fragDir.x, fragDir.y, fragDir.z);
	gl_FragColor = vec4(check,1);

	//gl_FragColor = vec4(0,0,0.5, 1.0);
	//gl_FragColor = vec4(-fragDir, 1.0);
}





