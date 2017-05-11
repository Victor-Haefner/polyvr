#include "VRAsphalt.h"
#include "core/objects/material/VRTextureGenerator.h"
#include "core/math/path.h"

#define GLSL(shader) #shader

using namespace OSG;

VRAsphalt::VRAsphalt() : VRMaterial("asphalt") {
    setVertexShader(asphalt_vp, "asphaltVP");
    setFragmentShader(asphalt_fp, "asphaltFP");
    setFragmentShader(asphalt_dfp, "asphaltDFP", true);
    clearTransparency();
    setShininess(128);
    clearTexture();
}

VRAsphalt::~VRAsphalt() {}

VRAsphaltPtr VRAsphalt::create() { return VRAsphaltPtr( new VRAsphalt() ); }

void VRAsphalt::clearTexture() {
    texGen = VRTextureGenerator::create();
	texGen->setSize(Vec3i(256,256,1), false);
	texGen->drawFill(Vec4f(0,0,0,1));
    roadData.clear();
    updateTexture();
}

VRTexturePtr VRAsphalt::noiseTexture() {
    VRTextureGenerator tg;
    tg.setSize(Vec3i(512, 512, 1));
    tg.add(PERLIN, 1.0, Vec3f(), Vec3f(0.6, 0.6, 0.6));
    tg.add(PERLIN, 1.0/2, Vec3f(), Vec3f(0.7, 0.7, 0.7));
    tg.add(PERLIN, 1.0/4, Vec3f(), Vec3f(0.8, 0.8, 0.8));
    tg.add(PERLIN, 1.0/8, Vec3f(), Vec3f(0.9, 0.9, 0.9));
    tg.add(PERLIN, 1.0/16, Vec3f(), Vec3f(1.0, 1.0, 1.0));
    return tg.compose(0);
}

VRTexturePtr VRAsphalt::mudTexture() {
    VRTextureGenerator tg;
    tg.setSize(Vec3i(256, 256, 1), 1);
    tg.add(PERLIN, 5, Vec4f(0.1, 0.1, 0.1, 1.0), Vec4f(1.0, 1.0, 1.0, 1.0));
    tg.add(PERLIN, 0.2, Vec4f(), Vec4f(0.8, 0.6, 0.4, 1.0));
    tg.add(PERLIN, 0.1, Vec4f(), Vec4f(0.4, 0.4, 0.4, 0.4));
    return tg.compose(1);
}

void VRAsphalt::updateTexture() {
    auto paths = texGen->compose(0);
    auto noise = noiseTexture();
    auto mud = mudTexture();
    setTexture(paths, false, 0);
    setTexture(noise, false, 1);
    setTexture(mud, false, 2);
    setMagMinFilter(GL_LINEAR, GL_LINEAR, 0);
    setMagMinFilter(GL_LINEAR, GL_LINEAR, 1);
    setMagMinFilter(GL_LINEAR, GL_LINEAR, 2);
    setShaderParameter("texMarkings", 0);
	setShaderParameter("texNoise", 1);
	setShaderParameter("texMud", 2);
}

void VRAsphalt::addPath(pathPtr path, int rID, float width, int dashN) {
    int& i = roadData[rID].rDataLengths;
    int iNpnts = i;
    i += 1;

    path->approximate(2);
    auto pnts = path->getPoints();
    int N = (pnts.size()-1)*0.5;

    for (int j = 0; j<N; j++) {// p1,p2,p3
        Vec3f P0 = pnts[2*j].pos();
        Vec3f P1 = pnts[2*j+1].pos();
        Vec3f P2 = pnts[2*j+2].pos();

        Vec3f A = P2 + P0 - P1*2;
        Vec3f B = (P1 - P0) * 2;
        Vec3f C = P0;

        texGen->drawPixel( Vec3i(rID, i+0, 0), Vec4f(A[0], A[1], A[2], 1.0) );
        texGen->drawPixel( Vec3i(rID, i+1, 0), Vec4f(B[0], B[1], B[2], 1.0) );
        texGen->drawPixel( Vec3i(rID, i+2, 0), Vec4f(C[0], C[1], C[2], 1.0) );
        i += 3;
    }

    texGen->drawPixel(Vec3i(rID,iNpnts,0), Vec4f(i-iNpnts-1,width,dashN,1));
    if (texGen->getSize()[0] < i) cout << "WARNING, texture width not enough!";
    if (texGen->getSize()[1] < rID) cout << "WARNING, texture height not enough!";
}

void VRAsphalt::addMarking(int rID, pathPtr marking, float width, int dashN) {
    if (roadData.count(rID) == 0) roadData[rID] = road();
    roadData[rID].markingsN++;
    texGen->drawPixel(Vec3i(rID,0,0), Vec4f(roadData[rID].markingsN, roadData[rID].tracksN, 0, 1));
    addPath(marking, rID, width, dashN);
}

void VRAsphalt::addTrack(int rID, pathPtr track, float width, int dashN) {
    if (roadData.count(rID) == 0) roadData[rID] = road();
    roadData[rID].tracksN++;
    texGen->drawPixel(Vec3i(rID,0,0), Vec4f(roadData[rID].markingsN, roadData[rID].tracksN, 0, 1));
    addPath(track, rID, width, dashN);
}


string VRAsphalt::asphalt_vp =
"#version 400 compatibility\n"
GLSL(

in vec4 osg_Vertex;
in vec2 osg_MultiTexCoord0;
in vec2 osg_MultiTexCoord1;

out vec4 position;
out vec2 tc1;
flat out int rID;

void main(void) {
	position = gl_ModelViewMatrix * osg_Vertex;
	gl_Position = gl_ModelViewProjectionMatrix * osg_Vertex;
	tc1 = osg_MultiTexCoord0;
	rID = int(osg_MultiTexCoord1.x);
}
);


string VRAsphalt::asphalt_fp =
"#version 400 compatibility\n"
"#extension GL_ARB_texture_rectangle : require\n"
"#extension GL_ARB_texture_rectangle : enable\n"
GLSL(
uniform sampler2D texMarkings;
uniform sampler2D texMud;
uniform sampler2D texNoise;

const float Inv3 = 1.0/3.0;
const float Inv27 = 1.0/27.0;

vec4 color = vec4(0.0,0.0,0.0,1.0);
vec4 trackColor = vec4(0.3, 0.3, 0.3, 1.0);
vec4 roadData;
bool doLine = false;
bool doTrack = false;

struct Point {
	vec3 pos;
	vec3 norm;
};

void debugColors(float y) {
	if (y<=0) color = vec4(1,0,0,1);
	if (y > 0 && y <= 1.0) color = vec4(0,1,y,1);
	if (y > 1) color = vec4(1,1,y*0.1,1);
}

vec4 getData(const int x, const int y) {
	return texelFetch(texMarkings, ivec2(x,y), 0);
}

float cuberoot(float x) {
	return sign(x)*pow(abs(x), Inv3);
}

// t3 + a t2 + b t + c
vec4 solveCubic(float a, float b, float c) {
	// compute depressed equation t3 + pt + q = 0
	float p = b - a*a*Inv3;
	double q = (2.0*a*a*a - 9.0*b*a) *Inv27 + c;
	float p3 = p*p*p;
	double d = q*q*0.25 + p3 *Inv27;
	float o = - a*Inv3;

	if (d >= 0.0) { // Single solution
		float u = cuberoot(float(-q*0.5 + sqrt(d)));
		float v = cuberoot(float(-q*0.5 - sqrt(d)));
		float r = o + u + v;
		return vec4(r, 0.0, 0.0, 1.0);
	}

	float u = 2.0*sqrt(-p*Inv3);
	float v = acos(3.0*float(q)/u/p)*Inv3;
	float r1 = o + u*cos(v);
	//float r2 = o + u*cos(v-2.0*3.14159265359*Inv3);
	float r3 = o + u*cos(v-4.0*3.14159265359*Inv3);
	float r2 = -r1-r3;
	return vec4(r1,r2,r3, 3.0);
}

vec3 toWorld(const vec3 p) {
	mat4 m = inverse(gl_ModelViewMatrix);
	return vec3(m*vec4(p,1.0));
}

const float pi = 3.14159265359;
const vec4 cLine = vec4(0.7, 0.5, 0.1, 1.0);
const vec3 light = vec3(-1,-1,-0.5);
const ivec3 off = ivec3(-1,0,1);
const vec2 size = vec2(1.5,0.0);
vec2 uv = vec2(0);
vec3 norm = vec3(0,1,0);
float noise = 1;

in vec4 position;
in vec2 tc1; // noise
flat in int rID;

void asphalt() {
	float g = 0.35+0.2*noise;
	color = vec4(g,g,g,1.0);
}

void applyMud() {
	vec4 c_mud = texture(texMud, uv);
	color = mix(color, c_mud, smoothstep( 0.3, 0.8, c_mud.a)*0.2 );
}

void applyLine() {
	float l = clamp(1.0 - smoothstep(0.0, 0.2, norm.x), 0, 1);
	color = mix(color, cLine, l );
}

void applyTrack(float d) {
	d = 0.5-d;
	d = clamp(d, 0.0, 1.0);
	color = mix(color, trackColor, d );
}

vec2 distToQuadBezier( vec3 A, vec3 B, vec3 C, vec3 x ) {
	// Bezier: At2 + Bt + C
	// Distance: kt3 + a*t2 + b*t + c
    float k = 2.0*dot(A,A);
    float a = 3.0*dot(A,B);
    float b = dot(B,B) + 2.0*dot(A,C-x);
    float c = dot(B,C-x);

    vec4 res;
    if (abs(k) <= 0.001) res = vec4(-c/b,0,0,1); // path is a line
	else res = solveCubic( a/k, b/k, c/k);

    float tmin = 1e20;
    float dmin = 1e20;
    for( int i=0; i<int(res[3]); i++ ) {
        float t = res[i];
        if( t>=0.0 && t <=1.0 ) {
            vec3 pos = A*t*t+B*t+C;
            float d = distance(pos,x);
            if (d < dmin) {
        		dmin = d;
        		tmin = t;
            }
        }
    }
    return vec2(dmin,tmin);
}

float distToPath(const int k, const int roadID, const vec3 pos, const vec4 pathData) {
	int Npoints = int(pathData.x); // testing
	float width2 = pathData.y*0.5;
	float dashL = pathData.z;

	vec3 A;
	vec3 B;
	vec3 C;
	int Nsegs = (Npoints-1)/2;
	for (int j=0; j<Nsegs; j++) {
		A = getData(roadID, k+3*j+1).xyz;
		B = getData(roadID, k+3*j+2).xyz;
		C = getData(roadID, k+3*j+3).xyz;
		vec2 dtmin = distToQuadBezier(A,B,C,pos);
		if (dtmin.x < width2 && dashL == 0) return dtmin.x;
		if (dtmin.x < width2 && int(dtmin.y*dashL)%2 == 0) return dtmin.x;
	}

	return 20.0;
}

void computeNormal() {
	vec2 tc = tc1*2;
    float s11 = texture(texNoise, tc).r;
    float s01 = textureOffset(texNoise, tc, off.xy).r;
    float s21 = textureOffset(texNoise, tc, off.zy).r;
    float s10 = textureOffset(texNoise, tc, off.yx).r;
    float s12 = textureOffset(texNoise, tc, off.yz).r;

    vec3 va = normalize(vec3(size.y,s21-s01,size.x));
    vec3 vb = normalize(vec3(size.x,s12-s10,size.y));
	norm = normalize( cross(va,vb) );
}

void computeDepth(bool doLine, bool doTrack) {
	float o = 0.0;
	if (doLine) o = -0.00002;
	if (doTrack) o = -0.00001;
	vec4 pp = gl_ProjectionMatrix * position;
	float d = (pp.z+o) / pp.w;
	gl_FragDepth = d*0.5 + 0.5;
}

void applyBlinnPhong() {
	vec3 n = gl_NormalMatrix * norm;
	vec3  light = normalize( gl_LightSource[0].position.xyz );// directional light
	float NdotL = max(dot( n, light ), 0.0);
	vec4  ambient = gl_LightSource[0].ambient * color;
	vec4  diffuse = gl_LightSource[0].diffuse * NdotL * color;
	float NdotHV = max(dot(norm, normalize(gl_LightSource[0].halfVector.xyz)),0.0);
	vec4  specular = gl_LightSource[0].specular * pow( NdotHV, gl_FrontMaterial.shininess );
	gl_FragColor = ambient + diffuse + specular;
}

void doPaths() {
    vec3 pos = toWorld(position.xyz);
	int k = 1;
	float dist = 1.0;
	int Nlines = int(roadData.x);

	for (int i=0; i<Nlines; i++) {
		vec4 pathData = getData(rID, k);
		float width = pathData.y;
		int Npoints = int(pathData.x);
		dist = distToPath(k, rID, pos, pathData);
		doLine = bool(dist < 10.0);
		if (doLine) break;
		k += Npoints+1;
	}

	if (!doLine) {
		int Ntracks = int(roadData.y);
		for (int i=0; i<Ntracks; i++) {
			vec4 pathData = getData(rID, k);
			float width = pathData.y;
			int Npoints = int(pathData.x);
			float d = distToPath(k, rID, pos, pathData);
			if (d < 10.0) {
				doTrack = true;
				dist = min(dist,d/width);
			}
			k += Npoints+1;
		}
	}

	if (doLine) applyLine();
	if (doTrack) applyTrack(dist);
}

void main(void) {
	uv = tc1.xy;
	roadData = getData(rID, 0);
	noise = texture(texNoise, uv*0.2).r;
	computeNormal();
	asphalt();
	doPaths();
	applyMud();
	//color = texture(texMarkings, uv);
	//color = texture(texMud, uv);
	//color = texture(texNoise, uv);

	norm = gl_NormalMatrix * norm;
    computeDepth(doLine,doTrack);

    applyBlinnPhong();
}
);


string VRAsphalt::asphalt_dfp =
"#version 400 compatibility\n"
"#extension GL_ARB_texture_rectangle : require\n"
"#extension GL_ARB_texture_rectangle : enable\n"
GLSL(
uniform sampler2D texMarkings;
uniform sampler2D texMud;
uniform sampler2D texNoise;

const float Inv3 = 1.0/3.0;
const float Inv27 = 1.0/27.0;

vec4 color = vec4(0.0,0.0,0.0,1.0);
vec4 trackColor = vec4(0.3, 0.3, 0.3, 1.0);

struct Point {
	vec3 pos;
	vec3 norm;
};

void debugColors(float y) {
	if (y<=0) color = vec4(1,0,0,1);
	if (y > 0 && y <= 1.0) color = vec4(0,1,y,1);
	if (y > 1) color = vec4(1,1,y*0.1,1);
}

vec4 getData(const int x, const int y) {
	return texelFetch(texMarkings, ivec2(x,y), 0);
}

float cuberoot(float x) {
	return sign(x)*pow(abs(x), Inv3);
}

// t3 + a t2 + b t + c
vec4 solveCubic(float a, float b, float c) {
	// compute depressed equation t3 + pt + q = 0
	float p = b - a*a*Inv3;
	double q = (2.0*a*a*a - 9.0*b*a) *Inv27 + c;
	float p3 = p*p*p;
	double d = q*q*0.25 + p3 *Inv27;
	float o = - a*Inv3;

	if (d >= 0.0) { // Single solution
		float u = cuberoot(float(-q*0.5 + sqrt(d)));
		float v = cuberoot(float(-q*0.5 - sqrt(d)));
		float r = o + u + v;
		return vec4(r, 0.0, 0.0, 1.0);
	}

	float u = 2.0*sqrt(-p*Inv3);
	float v = acos(3.0*float(q)/u/p)*Inv3;
	float r1 = o + u*cos(v);
	//float r2 = o + u*cos(v-2.0*3.14159265359*Inv3);
	float r3 = o + u*cos(v-4.0*3.14159265359*Inv3);
	float r2 = -r1-r3;
	return vec4(r1,r2,r3, 3.0);
}

vec3 toWorld(const vec3 p) {
	mat4 m = inverse(gl_ModelViewMatrix);
	return vec3(m*vec4(p,1.0));
}

const float pi = 3.14159265359;
const vec4 cLine = vec4(0.7, 0.5, 0.1, 1.0);
const vec3 light = vec3(-1,-1,-0.5);
const ivec3 off = ivec3(-1,0,1);
const vec2 size = vec2(1.5,0.0);
vec2 uv = vec2(0);
vec3 norm = vec3(0,1,0);
float noise = 1;

in vec4 position;
in vec2 tc1; // noise
flat in int rID;

void asphalt() {
	float g = 0.35+0.2*noise;
	color = vec4(g,g,g,1.0);
}

void applyMud() {
	vec4 c_mud = texture(texMud, uv);
	color = mix(color, c_mud, smoothstep( 0.3, 0.8, c_mud.a)*0.2 );
}

void applyLine() {
	float l = clamp(1.0 - smoothstep(0.0, 0.2, norm.x), 0, 1);
	color = mix(color, cLine, l );
}

void applyTrack(float d) {
	d = 0.5-d;
	d = clamp(d, 0.0, 1.0);
	color = mix(color, trackColor, d );
}

vec2 distToQuadBezier( vec3 A, vec3 B, vec3 C, vec3 x ) {
	// Bezier: At2 + Bt + C
	// Distance: kt3 + a*t2 + b*t + c
    float k = 2.0*dot(A,A);
    float a = 3.0*dot(A,B);
    float b = dot(B,B) + 2.0*dot(A,C-x);
    float c = dot(B,C-x);

    vec4 res;
    if (abs(k) <= 0.001) res = vec4(-c/b,0,0,1); // path is a line
	else res = solveCubic( a/k, b/k, c/k);

    float tmin = 1e20;
    float dmin = 1e20;
    for( int i=0; i<int(res[3]); i++ ) {
        float t = res[i];
        if( t>=0.0 && t <=1.0 ) {
            vec3 pos = A*t*t+B*t+C;
            float d = distance(pos,x);
            if (d < dmin) {
        		dmin = d;
        		tmin = t;
            }
        }
    }
    return vec2(dmin,tmin);
}

float distToPath(const int k, const int roadID, const vec3 pos, const vec4 pathData) {
	int Npoints = int(pathData.x); // testing
	float width2 = pathData.y*0.5;
	float dashL = pathData.z;

	vec3 A;
	vec3 B;
	vec3 C;
	int Nsegs = (Npoints-1)/2;
	for (int j=0; j<Nsegs; j++) {
		A = getData(roadID, k+3*j+1).xyz;
		B = getData(roadID, k+3*j+2).xyz;
		C = getData(roadID, k+3*j+3).xyz;
		vec2 dtmin = distToQuadBezier(A,B,C,pos);
		if (dtmin.x < width2 && dashL == 0) return dtmin.x;
		if (dtmin.x < width2 && int(dtmin.y*dashL)%2 == 0) return dtmin.x;
	}

	return 20.0;
}

void computeNormal() {
	vec2 tc = tc1*2;
    float s11 = texture(texNoise, tc).r;
    float s01 = textureOffset(texNoise, tc, off.xy).r;
    float s21 = textureOffset(texNoise, tc, off.zy).r;
    float s10 = textureOffset(texNoise, tc, off.yx).r;
    float s12 = textureOffset(texNoise, tc, off.yz).r;

    vec3 va = normalize(vec3(size.y,s21-s01,size.x));
    vec3 vb = normalize(vec3(size.x,s12-s10,size.y));
	norm = normalize( cross(va,vb) );
}

void computeDepth(bool doLine, bool doTrack) {
	vec4 o = vec4(0,0,0,0);
	if (doLine) o = vec4(0,0.02,0.02,0);
	if (doTrack) o = vec4(0,0.01,0.01,0);
	vec4 pp = gl_ProjectionMatrix * (position + o);
	float d = pp.z / pp.w;
	gl_FragDepth = d*0.5 + 0.5;
}

void main(void) {
    vec3 pos = toWorld(position.xyz);

	int roadID = rID;
	uv = tc1.xy;
	vec4 roadData = getData(roadID, 0);

	noise = texture(texNoise, uv*0.2).r;
	computeNormal();
	asphalt();

	int k = 1;
	float dist = 1.0;
	int Nlines = int(roadData.x);
	bool doLine = false;
	bool doTrack = false;

	for (int i=0; i<Nlines; i++) {
		vec4 pathData = getData(roadID, k);
		float width = pathData.y;
		int Npoints = int(pathData.x);
		dist = distToPath(k, roadID, pos, pathData);
		doLine = bool(dist < 10.0);
		if (doLine) break;
		k += Npoints+1;
	}

	if (!doLine) {
		int Ntracks = int(roadData.y);
		for (int i=0; i<Ntracks; i++) {
			vec4 pathData = getData(roadID, k);
			float width = pathData.y;
			int Npoints = int(pathData.x);
			float d = distToPath(k, roadID, pos, pathData);
			if (d < 10.0) {
				doTrack = true;
				dist = min(dist,d/width);
			}
			k += Npoints+1;
		}
	}

	if (doLine) applyLine();
	if (doTrack) applyTrack(dist);

	applyMud();
	//color = texture(texMarkings, uv);
	//color = texture(texMud, uv);
	//color = texture(texNoise, uv);

	norm = gl_NormalMatrix * norm;
    computeDepth(doLine,doTrack);

    gl_FragData[0] = vec4(position.xyz/position.w, 1.0);
    gl_FragData[1] = vec4(norm, 1);
    gl_FragData[2] = color;
}
);

