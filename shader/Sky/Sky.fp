#version 400 compatibility
#define M_PI 3.1415926535897932384626433832795

in vec3 norm;
in vec2 tcs;
in mat4 miMV;

uniform sampler2D tex;
uniform vec2 OSGViewportSize;

vec3 sunDir = vec3(1.0, 1.0, 1.0); //define sun direction
	
vec3 fragDir;
vec2 angles;

vec4 color;
vec4 colAmbient = vec4(0.1 , 0.12, 0.3, 1); // get colour val from charts
vec3 colLuminance = vec3(0.11); // 
vec3 colClouds = vec3(0.9);

float cloudHeight = 1000.;
float cloudScale = 1e-6;

void computeDirection() { 
	float aspect = OSGViewportSize.y/OSGViewportSize.x;
	fragDir = vec3(tcs*2-vec2(1),-1);
	fragDir.x /= aspect;
	fragDir = (miMV*vec4(normalize(fragDir),0)).xyz;
}

//void computeAngles() {
//	// in radians?
//	angles.x = atan(fragDir[2], fragDir[0]);
//	angles.y = atan(fragDir[1], length(vec2(fragDir[0],fragDir[2])));
//}

float computeLuminanceDaylight() {
	/// TEST TEXTURE
	//float col = texture(tex,abs(angles)).x;
	//if (angles.y < 0) col -= abs(col-0.5); // identifies ground
	//vec4 colour = vec4(abs(angles)*col,0, 1.0);

	//vec3 colour = vec3(0.2 , 0.32487, 0.38008); // get colour val from charts
	
	sunDir = normalize(sunDir);
	//fragDir = normalize(fragDir); // is this necessary?
	
	/// LUMINANCE
	
	// luminance model (CIE - from preetham)
	// Y_z \frac{ (0.91 + 10 exp(- 3 \gamma) + 0.45 cos^2 \gamma) 
	// (1 - exp(-0.32 / cos \theta) ) }
	// { (0.91 + 10 exp(- 3 \theta_s) + 0.45 cos^2 \theta_s) 
	// (1 - exp(-0.32) ) }
	// Y_z is luminance at zenith
	float y_z = 0.8; // get from table?
	// \gamma is angle between viewing direction (fragDir) and sun (sunDir)
	float gamma = acos(dot(fragDir, sunDir));
	// \theta is angle of fragDir to zenith
	float theta = acos(fragDir[1]);
	// \theta_s is angle of sun to zenith
	float theta_s = acos(sunDir[1]);
	
	float y1 = (0.91 + 10 * exp(- 3 * gamma) + 0.45 * pow(cos(gamma), 2));
	float y2 = (1 - exp(- 0.32 / cos(theta)));
	float y3 = (0.91 + 10 * exp(- 3 * theta_s) + 0.45 * pow(cos(theta_s), 2)) * (1 - exp(-0.32));
	return y_z * y1 * y2 / y3;
}

float computeLuminanceOvercast() {
	// overcast luminance model (CIE - from preetham)
	// Y_z(1 + 2 cos \theta) / 3
	float y_z = 0.8;
	return y_z * (1 + 2 * fragDir[1]) / 3;
}

// returns the x-z coordinates on plane y = h
// at which the ray fragDir intersects
vec2 planeIntersect(float h){
	vec2 pt = vec2(0.);
	// r = h / cos \phi
	pt = - h * fragDir.xz / fragDir.y ;
	return pt;
}

float modifyCloudLuminance(float h) {
	// r = h / cos phi
	// shrink range according to r
	float factor = 0.0;
	return factor;
}

// uses noise texture to generate cloud cover
// this should overlay base colour
// put overcast luminance model here?
void computeClouds() {
	vec2 lowerMap = planeIntersect(cloudHeight);
	float lowerClouds = texture(tex, cloudScale * lowerMap).x;
	vec2 upperMap = planeIntersect(1.1 * cloudHeight);
	float upperClouds = texture(tex, 0.9 * cloudScale * upperMap).x;
	// compute luminance of clouds based on angle
	float y = computeLuminanceOvercast();
	
	if (fragDir[1] > 0) {
		// write higher layers of cloud first
		//upperClouds = smoothstep( 0.8, 1.0, upperClouds);
		color = mix(vec4(y * 2 * colClouds, 1.0), color, upperClouds);
		// overlay darker base layer last
		lowerClouds = smoothstep( 0.7, 1.0, lowerClouds);
		color = mix(vec4(y * colClouds, 1.0), color, lowerClouds);
	}
}

void addSun() {
	float sunAngSize = 0.00873; // define sun size in radians
	//sun angular size ~0.5 degrees = 0.00873 rad http://astronomy.swin.edu.au/cosmos/A/Angular+Diameter
	// if angle of fragment to sun is small enough, paint white
	float gamma = acos(dot(fragDir, sunDir));
	//if (gamma < sunAngSize) color = vec4(1.0, 1.0, 1.0, 1.0);
	color = mix(vec4(1.0, 1.0, 1.0, 1.0), color, smoothstep( sunAngSize * 0.7, sunAngSize * 1.0, gamma));
}

void main() {
	computeDirection();

	// luminance model for sky
	float L = computeLuminanceDaylight();
	color = colAmbient + vec4( L * colLuminance, 1 );

	// add sun
	addSun();
	
	// add cloud cover
	computeClouds();

	gl_FragColor = color;
    gl_FragDepth = 1.0; // depth is infinite at 1.0? behind all else (check)
}
