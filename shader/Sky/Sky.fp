#version 400 compatibility
#define M_PI 3.1415926535897932384626433832795

in vec3 norm;
in vec2 tcs;
in mat4 miMV;

uniform sampler2D tex;
uniform vec2 OSGViewportSize;

uniform vec3 sunPos; //define sun direction
uniform float theta_s;
uniform float turbidity;

uniform vec2 cloudOffset; //to shift cloud texture
uniform float cloudScale; // 1e-5
uniform float cloudDensity;
uniform float cloudHeight; // 1000.

vec3 sunDir = vec3(1., 0., 0.);
vec3 fragDir;
//vec2 angles;
vec4 color;

float x_z;
float y_z;
float Y_z;

//vec4 colAmbient = vec4(0.1 , 0.12, 0.3, 1); // get colour val from charts
//vec3 colLuminance = vec3(0.11);
vec3 colLuminance = vec3(0.13906 , 0.24297, 0.34252);
vec3 colClouds = vec3(0.9);


void computeDirection() { 
	float aspect = OSGViewportSize.y/OSGViewportSize.x;
	float l = -1/tan(0.5); // assumes total screen height of 2
	fragDir = vec3(tcs*2-vec2(1),l);
	fragDir.x /= aspect;
	fragDir = (miMV*vec4(normalize(fragDir),0)).xyz;
}

void zenithColor() {
	float chi = ((4. / 9.) - (turbidity / 120.) ) * ( M_PI - 2 * theta_s );
	Y_z = (4.0453 * turbidity - 4.9710) * tan(chi) - 0.2155 * turbidity + 2.4192; // get luminance

	vec4 vTheta_s = vec4(theta_s*theta_s*theta_s, theta_s*theta_s, theta_s, 1);
	vec4 vTurbidities = vec4(turbidity*turbidity, turbidity, 1, 0);

	// this format in row-major order, all others column major
	mat4 x_mat = mat4( 0.0017, -0.0037, 0.0021, 0.000,
                          -0.0290, 0.0638, -0.0320, 0.0039,
                           0.1169, -0.2120, 0.0605, 0.2589,
                           0., 0., 0., 0.); 
	mat4 y_mat = mat4(0.0028, -0.0061, 0.0032, 0.000,
                         -0.0421, 0.0897, -0.0415, 0.0052,
                          0.1535, -0.2676, 0.0667, 0.2669,
                          0., 0., 0., 0.);

	vec4 tmp = x_mat * vTheta_s;
	x_z = dot(vTurbidities, tmp);
	tmp = y_mat * vTheta_s;
	y_z = dot(vTurbidities, tmp);
}

vec3 xyYRGB(float x, float y, float Y) {
	float X = x * Y / y;
	float Z = (1. - x - y) * Y / y;

	// scale XYZ
	X = 0.0125313*(X - 0.1901);
	Y = 0.0125313*(Y - 0.2);
	Z = 0.0125313*(Z - 0.2178);

	mat4 m_inv = mat4(2.3706743, -0.9000405, -0.4706338, 0., 
			-0.5138850,  1.4253036,  0.0885814, 0.,
			0.0052982, -0.0146949,  1.0093968, 0., 
			0., 0., 0., 0.);

	vec4 tmp = vec4(X, Y, Z, 0.);
	vec4 rgb;

	rgb = m_inv * tmp;

	// add non-linearity
	for (int i=0; i<3; ++i) {
		if (rgb[i] > 0.0031308) {
			rgb[i] = 1.055 * pow(rgb[i], 1./2.4) - 0.055;
		} else {
			rgb[i] = 12.92 * rgb[i];
		}
	}

	return vec3(rgb[0], rgb[1], rgb[2]);
}

float computeLuminanceDaylight(float Y) {
	sunDir = normalize(sunPos);
	
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
	//return Y_z * (1 + 2 * fragDir[1]) / 3;
	float factor = 0.2;
	return Y_z * factor * (1 + 0.2 * fragDir[1]);

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


void computeClouds() {
	float density = cloudDensity;
	density = 0.3;
	float scale = cloudScale;
	scale = 1e-4;

	vec2 lowerMap = planeIntersect(cloudHeight);
	float lowerClouds = texture(tex, scale * lowerMap + cloudOffset).x;
	vec2 upperMap = planeIntersect(1.5 * cloudHeight);
	float upperClouds = texture(tex, -0.9 * scale * upperMap - cloudOffset*0.7).x;
	// compute luminance of clouds based on angle
	float y = computeLuminanceOvercast();
	
	if (fragDir[1] > 0) {
		// write higher layers of cloud first
		upperClouds = smoothstep( 0.25*density, 0.75*density + 0.5, 0.5*upperClouds);
		color = mix(vec4(y * colClouds, 1.0), color, upperClouds);
		// overlay darker base layer last
		lowerClouds = smoothstep( 0.5*density, 0.5*density + 0.5, 0.5 * lowerClouds);
		color = mix(vec4(y * colClouds, 1.0), color, lowerClouds);
		//color = vec4(lowerClouds, lowerClouds, lowerClouds, 1.0);
	}
}

void addSun() {
	float sunAngSize = 0.00873; // define sun size in radians
	//sun angular size ~0.5 degrees = 0.00873 rad http://astronomy.swin.edu.au/cosmos/A/Angular+Diameter
	// if angle of fragment to sun is small enough, paint white
	float gamma = acos(dot(fragDir, sunDir));
	//if (gamma < sunAngSize) color = vec4(1.0, 1.0, 1.0, 1.0);
	if (sunDir[1] > 0) color = mix(vec4(1.0, 1.0, 1.0, 1.0), color, smoothstep( sunAngSize * 0.7, sunAngSize * 1.0, gamma));
}

void addGround() {
	if (fragDir[1] < 0) color = vec4(0.,0.,0.,1.);
}

void main() {

	computeDirection();

	zenithColor();

	// luminance model for sky
	float Y = computeLuminanceDaylight(Y_z);

	float brightness_factor = 2;
	color = vec4(xyYRGB(x_z,y_z,Y), 1);
	color.xyz *= brightness_factor;
	color.x *= 0.7;
	color.y *=(1.4 - 0.4*color.y);
	color.xyz += 0.4*(vec3(1) - 1*color.xyz);



	
	// add cloud cover
	computeClouds();

	// add sun
	addSun();
	addGround();

	gl_FragColor = color;
	gl_FragDepth = 1.0; // depth is infinite at 1.0? behind all else (check)
}
