#include "VRAsphalt.h"
#include "core/objects/material/VRTextureGenerator.h"
#include "core/objects/material/VRTexture.h"
#include "core/math/path.h"
#include "core/utils/VRTimer.h"
#include "core/utils/toString.h"

#define GLSL(shader) #shader

using namespace OSG;


VRAsphalt::VRAsphalt() : VRMaterial("asphalt") {}

VRAsphalt::~VRAsphalt() {}

VRAsphaltPtr VRAsphalt::create() {
    auto p = VRAsphaltPtr( new VRAsphalt() );
    p->init();
    VRMaterial::materials[p->getName()] = p;
    return p;
}

void VRAsphalt::init() {
    VRMaterial::init();
    asphalt_fp = asphalt_fp_head + asphalt_fp_core;
    asphalt_dfp = asphalt_fp_head + asphalt_dfp_core;

    setVertexShader(asphalt_vp, "asphaltVP");
    setFragmentShader(asphalt_fp, "asphaltFP");
    setFragmentShader(asphalt_dfp, "asphaltDFP", true);
    clearTransparency();
    setShininess(128);
    clearTexture();
    setMarkingsColor(Color4f(0.7,0.7,0.7,1.0), 1);
    setMarkingsColor(Color4f(0.9,0.5,0.1,1.0), 2);
}

void VRAsphalt::setArrowMaterial() {
    setFragmentShader(asphaltArrow_fp_head + asphaltArrow_fp_core , "asphaltArrowFP");
    setFragmentShader(asphaltArrow_fp_head + asphaltArrow_dfp_core, "asphaltArrowDFP", true);
}

void VRAsphalt::setMarkingsColor(Color4f c, int ID) {
    setShaderParameter("cLine"+toString(ID), c);
}

void VRAsphalt::clearTexture() {
    texGen = VRTextureGenerator::create();
	//texGen->setSize(Vec3i(4096,8192,1), true); // Number of roads, Number of markings per road
	texGen->setSize(Vec3i(4096,1600,1), true); // Number of roads, Number of markings per road
	texGen->drawFill(Color4f(0,0,0,1));
    roadData.clear();
    updateTexture();
}

VRTexturePtr VRAsphalt::noiseTexture() {
    VRTextureGenerator tg;
    tg.setSize(Vec3i(512, 512, 1));
    tg.add(PERLIN, 1.0, Color4f(), Color4f(0.6, 0.6, 0.6, 1));
    tg.add(PERLIN, 1.0/2, Color4f(), Color4f(0.7, 0.7, 0.7, 1));
    tg.add(PERLIN, 1.0/4, Color4f(), Color4f(0.8, 0.8, 0.8, 1));
    tg.add(PERLIN, 1.0/8, Color4f(), Color4f(0.9, 0.9, 0.9, 1));
    tg.add(PERLIN, 1.0/16, Color4f(), Color4f(1.0, 1.0, 1.0, 1));
    return tg.compose(0);
}

VRTexturePtr VRAsphalt::mudTexture() {
    VRTextureGenerator tg;
    tg.setSize(Vec3i(256, 256, 1), 1);
    tg.add(PERLIN, 5, Color4f(0.1, 0.1, 0.1, 1.0), Color4f(1.0, 1.0, 1.0, 1.0));
    tg.add(PERLIN, 0.2, Color4f(), Color4f(0.8, 0.6, 0.4, 1.0));
    tg.add(PERLIN, 0.1, Color4f(), Color4f(0.4, 0.4, 0.4, 0.4));
    return tg.compose(1);
}

void VRAsphalt::updateTexture() {
    auto setupTexture = [&](int unit, VRTexturePtr tex, string var) {
        setTexture(tex, false, unit);
        setMagMinFilter(GL_LINEAR, GL_LINEAR, unit);
        setShaderParameter(var, unit);
    };

    VRTimer t; t.start(); // TODO: increase PERLIN performance!
    //cout << "VRAsphalt::updateTexture performance:\n";
    pathTex = texGen->compose(0); //cout << " paths: " << t.stop() << endl;
    if (!noiseTex) noiseTex = noiseTexture(); //cout << " noiseTexture: " << t.stop() << endl;
    if (!mudTex) mudTex = mudTexture(); //cout << " mudTexture: " << t.stop() << endl;

    setupTexture(0, pathTex, "texMarkings");
    setupTexture(1, noiseTex, "texNoise");
    setupTexture(2, mudTex, "texMud");
}

void VRAsphalt::addPath(PathPtr path, int rID, float width, float dashL, float offset, int colorID) {
    int& i = roadData[rID].rDataLengths;
    int iNpnts = i;
    i += 2;

    path->approximate(2);
    auto pnts = path->getPoints();
    int N = (pnts.size()-1)*0.5;

    if (width < 1e-3) {
        cout << " Warning in VRAsphalt::addPath, width way too small, N: " << N << "  rID: " << rID << " width: " << width << " dashL: " << dashL << " offset: " << offset << " colorID: " << colorID << endl;
    }

    for (int j = 0; j<N; j++) {// p1,p2,p3
        Vec3d P0 = pnts[2*j].pos();
        Vec3d P1 = pnts[2*j+1].pos();
        Vec3d P2 = pnts[2*j+2].pos();

        Vec3d A = P2 + P0 - P1*2;
        Vec3d B = (P1 - P0) * 2;
        Vec3d C = P0;

        float k, a, b;
        k = 2.0*A.dot(A);
        a = 3.0*A.dot(B);
        b = B.dot(B);

        float dashS = dashL > 0 ? path->getLength(2*j, 2*j+2)/dashL : 0;

        texGen->drawPixel( Vec3i(rID, i+0, 0), Color4f(A[0], A[1], A[2], 1.0) );
        texGen->drawPixel( Vec3i(rID, i+1, 0), Color4f(B[0], B[1], B[2], 1.0) );
        texGen->drawPixel( Vec3i(rID, i+2, 0), Color4f(C[0], C[1], C[2], 1.0) );
        texGen->drawPixel( Vec3i(rID, i+3, 0), Color4f(k, a, b, dashS) );
        i += 4;
    }

    int Npoints = i-iNpnts-2;
    texGen->drawPixel(Vec3i(rID,iNpnts  ,0), Color4f(Npoints,width,colorID,1));
    texGen->drawPixel(Vec3i(rID,iNpnts+1,0), Color4f(offset,0,0,1));
    if (texGen->getSize()[0] < rID) cout << "WARNING, texture width not enough! " << rID << "/" << texGen->getSize()[0] << endl;
    if (texGen->getSize()[1] < i) cout << "WARNING, texture height not enough! " << i << "/" << texGen->getSize()[1] << endl;
}

void VRAsphalt::addMarking(int rID, PathPtr marking, float width, float dashL, float offset, int colorID) {
    if (roadData.count(rID) == 0) roadData[rID] = road();
    auto& rdata = roadData[rID];
    rdata.markingsN++;
    texGen->drawPixel(Vec3i(rID,0,0), Color4f(rdata.markingsN, rdata.tracksN, 0, 1));
    addPath(marking, rID, width, dashL, offset, colorID);
}

void VRAsphalt::addTrack(int rID, PathPtr track, float width, float dashL, float offset) {
    if (roadData.count(rID) == 0) roadData[rID] = road();
    auto& rdata = roadData[rID];
    rdata.tracksN++;
    texGen->drawPixel(Vec3i(rID,0,0), Color4f(rdata.markingsN, rdata.tracksN, 0, 1));
    addPath(track, rID, width, dashL, offset);
}

double VRAsphalt::getMemoryConsumption() {
    double res = sizeof(*this);
    res += pathTex->getByteSize();
    res += noiseTex->getByteSize();
    res += mudTex->getByteSize();
    //res += texGen->getMemoryConsumption();
    return res;
}


string VRAsphalt::asphalt_vp =
"#version 400 compatibility\n"
GLSL(

in vec4 osg_Vertex;
in vec4 osg_Color;
in vec2 osg_MultiTexCoord0;
in vec2 osg_MultiTexCoord1;

out vec4 position;
out vec2 tc1;
out vec2 tc2;
out vec3 col;
flat out int rID;

void main(void) {
	position = gl_ModelViewMatrix * osg_Vertex;
	gl_Position = gl_ModelViewProjectionMatrix * osg_Vertex;
	tc1 = osg_MultiTexCoord0;
	tc2 = osg_MultiTexCoord1;
	col = osg_Color.rgb;
	rID = int(osg_MultiTexCoord1.x);
}
);


string VRAsphalt::asphalt_fp = "";
string VRAsphalt::asphalt_dfp = "";

string VRAsphalt::asphalt_fp_head =
"#version 400 compatibility\n"
"#extension GL_ARB_texture_rectangle : require\n"
"#extension GL_ARB_texture_rectangle : enable\n"
GLSL(
uniform sampler2D texMarkings;
uniform sampler2D texMud;
uniform sampler2D texNoise;
uniform vec4 cLine1;
uniform vec4 cLine2;

const float Inv3 = 1.0/3.0;
const float Inv27 = 1.0/27.0;

vec4 color = vec4(0.0,0.0,0.0,1.0);
vec4 trackColor = vec4(0.3, 0.3, 0.3, 1.0);

vec4 roadData;
bool doLine = false;
bool doTrack = false;
float distTrack = 1.0;

struct Point {
	vec3 pos;
	vec3 norm;
};

const float pi = 3.14159265359;
const vec3 light = vec3(-1,-1,-0.5);
const ivec3 off = ivec3(-1,0,1);
const vec2 size = vec2(1.5,0.0);
vec2 uv = vec2(0);
vec3 norm = vec3(0,1,0);
float noise = 1;

in vec4 position;
in vec2 tc1; // noise
flat in int rID;

void debugColors(float y) {
	if (y<=0) color = vec4(1,0,0,1);
	if (y > 0 && y <= 1.0) color = vec4(0,1,y,1);
	if (y > 1) color = vec4(1,1,y*0.1,1);
}

vec4 getData(const int x, const int y) {
	return texelFetch(texMarkings, ivec2(x,y), 0);
}

vec3 toWorld(const vec3 p) {
	mat4 m = inverse(gl_ModelViewMatrix);
	return vec3(m*vec4(p,1.0));
}

void asphalt() {
	float g = 0.35+0.2*noise;
	color = vec4(g,g,g,1.0);
}

void applyMud() {
	vec4 c_mud = texture(texMud, uv);
	color = mix(color, c_mud, smoothstep( 0.3, 0.8, c_mud.a)*0.2 );
}

void applyLine(int colorID) {
	float l = clamp(1.0 - smoothstep(0.0, 0.2, norm.x), 0, 1);
	if (colorID == 2) color = mix(color, cLine2, l );
	else color = mix(color, cLine1, l );
}

void applyTrack() {
	distTrack = 0.5-distTrack;
	distTrack = clamp(distTrack, 0.0, 1.0);
	color = mix(color, trackColor, distTrack );
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

vec2 distToQuadBezier( vec3 A, vec3 B, vec3 C, vec3 D, vec3 x ) {
	// Bezier: At2 + Bt + C
	// Distance: kt3 + a*t2 + b*t + c
    float k = D[0];//2.0*dot(A,A);
    float a = D[1];//3.0*dot(A,B);
    float b = D[2] + 2.0*dot(A,C-x);//dot(B,B) + 2.0*dot(A,C-x);
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
            //float d = distance(pos,x);
            vec3 D = pos-x; D.y *= 0.02; // minor hack, reduces artifacts when road surface is clamped to terrain
            float d = length(D);
            if (d < dmin) {
        		dmin = d;
        		tmin = t;
            }
        }
    }

    return vec2(dmin,tmin);
}

float squareDistToSegment( vec3 A, vec3 B, vec3 x ) {
    vec3 d = B-A;
    float L2 = dot(d,d);
    float t = -dot(A-x,d)/L2;
    vec3 ps = A+d*t;
    if (t<0) ps = A;
    if (t>1) ps = B;
    return dot(ps-x,ps-x);
}

float distToQuadBezierHull( vec3 A, vec3 B, vec3 C, vec3 x ) {
	vec3 P0 = C;
	vec3 P1 = (C-A)*0.5;
	vec3 P2 = A+B+C;
	float d = 0;
	float d1 = squareDistToSegment(P0, P1, x);
	float d2 = squareDistToSegment(P1, P2, x);
	float d3 = squareDistToSegment(P2, P0, x);
	return min(min(d1,d2),d3);
}

float distToPath(const int k, const int roadID, const vec3 pos, const vec4 pathData1, const vec4 pathData2) {
	int Npoints = int(pathData1.x);
	float width2 = pathData1.y*0.5;
    float offset = pathData2.x;

	vec4 A;
	vec4 B;
	vec4 C;
	vec4 D;
	int Nsegs = int(Npoints*0.25);
	for (int j=0; j<Nsegs; j++) {
		A = getData(roadID, k+4*j+1);
		B = getData(roadID, k+4*j+2);
		C = getData(roadID, k+4*j+3);
		D = getData(roadID, k+4*j+4);


		float d1 = distToQuadBezierHull(A.xyz,B.xyz,C.xyz,pos);
		if (d1 < 100) {
			vec2 dtmin = distToQuadBezier(A.xyz,B.xyz,C.xyz,D.xyz,pos);
			if (dtmin.x > offset - width2 && dtmin.x < offset + width2) {
		        float dashL = D.w;
		        if (dashL == 0) return abs(dtmin.x - offset);
		        if (int(dtmin.y*dashL)%2 == 0) return abs(dtmin.x - offset);
			}
		}
	}

	return 20.0;
}

void computeNormal() {
	/*vec2 tc = tc1*2;
    float s11 = texture(texNoise, tc).r;
    float s01 = textureOffset(texNoise, tc, off.xy).r;
    float s21 = textureOffset(texNoise, tc, off.zy).r;
    float s10 = textureOffset(texNoise, tc, off.yx).r;
    float s12 = textureOffset(texNoise, tc, off.yz).r;

    vec3 va = normalize(vec3(size.y,s21-s01,size.x));
    vec3 vb = normalize(vec3(size.x,s12-s10,size.y));
	norm = normalize( cross(va,vb) );*/
	norm = vec3(0,1,0);
}

void computeDepth() {
	float o = 0.0;
	if (doLine) o = -0.00002;
	if (doTrack) o = -0.00001*distTrack;
	vec4 pp = gl_ProjectionMatrix * position;
	float d = (pp.z+o) / pp.w;
	gl_FragDepth = d*0.5 + 0.5;
}

void doPaths() {
    vec3 pos = toWorld(position.xyz);
	int k = 1;
	int Nlines = int(roadData.x);
	int colorID = 0;

	for (int i=0; i<Nlines; i++) {
		vec4 pathData1 = getData(rID, k);
		vec4 pathData2 = getData(rID, k+1);
		int Npoints = int(pathData1.x);
        colorID = int(pathData1.z);
        distTrack = distToPath(k+1, rID, pos, pathData1, pathData2);
        doLine = bool(distTrack < 10.0);
        if (doLine) break;
		k += Npoints+2;
	}

	if (doLine) { applyLine(colorID); return; }

	int Ntracks = int(roadData.y);
	for (int i=0; i<Ntracks; i++) {
		vec4 pathData1 = getData(rID, k);
        vec4 pathData2 = getData(rID, k+1);
		float width = pathData1.y;
		int Npoints = int(pathData1.x);
		float d = distToPath(k+1, rID, pos, pathData1, pathData2);
		if (d < 10.0) {
			doTrack = true;
			distTrack = min(distTrack,d/width);
		}
		k += Npoints+2;
	}

	if (doTrack) applyTrack();
}

);


string VRAsphalt::asphalt_fp_core =
GLSL(
void applyBlinnPhong() {
	vec3 n = normalize( gl_NormalMatrix * norm );
	vec3  light = normalize( gl_LightSource[0].position.xyz );// directional light
	float NdotL = max(dot( n, light ), 0.0);
	vec4  ambient = gl_LightSource[0].ambient * color;
	vec4  diffuse = gl_LightSource[0].diffuse * NdotL * color;
	float NdotHV = max(dot(norm, normalize(gl_LightSource[0].halfVector.xyz)),0.0);
	vec4  specular = gl_LightSource[0].specular * pow( NdotHV, gl_FrontMaterial.shininess );
	gl_FragColor = ambient + diffuse + specular;
}

void main(void) {
	uv = tc1.xy;
	roadData = getData(rID, 0);
	noise = texture(texNoise, uv*0.2).r;
	computeNormal();
	asphalt();
	doPaths();
	applyMud();
    computeDepth();
    applyBlinnPhong();
}
);


string VRAsphalt::asphalt_dfp_core =
GLSL(
void main(void) {
	uv = tc1.xy;
	roadData = getData(rID, 0);
	noise = texture(texNoise, uv*0.2).r;
	computeNormal();
	asphalt();
	doPaths();
	applyMud();
    computeDepth();
	//norm = normalize( gl_NormalMatrix * norm );
	norm = normalize( gl_NormalMatrix * norm ) + vec3(0,0,0.2); // bending normal towards camera to increase lightning
    gl_FragData[0] = vec4(position.xyz/position.w, 1.0);
    gl_FragData[1] = vec4(norm, 1);
    gl_FragData[2] = color;
}
);

string VRAsphalt::asphaltArrow_fp_head =
"#version 400 compatibility\n"
"#extension GL_ARB_texture_rectangle : require\n"
"#extension GL_ARB_texture_rectangle : enable\n"
GLSL(
uniform sampler2D texMarkings;
uniform sampler2D texMud;
uniform sampler2D texNoise;
uniform int NArrowTex;
uniform vec4 cLine1;
uniform vec4 cLine2;

vec4 color = vec4(0.0,0.0,0.0,1.0);
vec4 trackColor = vec4(0.3, 0.3, 0.3, 1.0);
bool doLine = false;

const float pi = 3.14159265359;
const vec3 light = vec3(-1,-1,-0.5);
const ivec3 off = ivec3(-1,0,1);
const vec2 size = vec2(1.5,0.0);
vec2 uv = vec2(0);
vec3 norm = vec3(0,1,0);
float noise = 1;

in vec4 position;
in vec2 tc1;
in vec2 tc2; // noise
in vec3 col;
flat in int rID;

void asphalt() {
	float g = 0.35+0.2*noise;
	color = vec4(g,g,g,1.0);
}

void applyMud() {
	vec4 c_mud = texture(texMud, uv);
	color = mix(color, c_mud, smoothstep( 0.3, 0.8, c_mud.a)*0.2 );
}

void applyLine(float L) {
	color = mix(vec4(0.5,0.5,0.5,1.0), cLine1, L );
}

void computeNormal() {
	/*vec2 tc = tc2*2;
    float s11 = texture(texNoise, tc).r;
    float s01 = textureOffset(texNoise, tc, off.xy).r;
    float s21 = textureOffset(texNoise, tc, off.zy).r;
    float s10 = textureOffset(texNoise, tc, off.yx).r;
    float s12 = textureOffset(texNoise, tc, off.yz).r;

    vec3 va = normalize(vec3(size.y, s21-s01, size.x));
    vec3 vb = normalize(vec3(size.x, s12-s10, size.y));
	norm = normalize( cross(va,vb) );*/
	norm = vec3(0,1,0);
}

void computeDepth() {
	float o = 0.0;
	if (doLine) o = -0.00002;
	vec4 pp = gl_ProjectionMatrix * position;
	float d = (pp.z+o) / pp.w;
	gl_FragDepth = d*0.5 + 0.5;
}

void drawArrow() {
	uv = tc2.xy;
	noise = texture(texNoise, uv*0.2).r;
	computeNormal();
	asphalt();

	vec2 tc = vec2( (tc1.x+col.x*1000)*(1.0/NArrowTex), tc1.y );
	float mark = texture(texMarkings, tc).r;
	if (mark < 0.1) discard;
	applyLine(mark);
	applyMud();
    computeDepth();
}

);

string VRAsphalt::asphaltArrow_fp_core =
GLSL(
void applyBlinnPhong() {
	vec3 n = normalize( gl_NormalMatrix * norm );
	vec3  light = normalize( gl_LightSource[0].position.xyz );// directional light
	float NdotL = max(dot( n, light ), 0.0);
	vec4  ambient = gl_LightSource[0].ambient * color;
	vec4  diffuse = gl_LightSource[0].diffuse * NdotL * color;
	float NdotHV = max(dot(norm, normalize(gl_LightSource[0].halfVector.xyz)),0.0);
	vec4  specular = gl_LightSource[0].specular * pow( NdotHV, gl_FrontMaterial.shininess );
	gl_FragColor = ambient + diffuse + specular;
}

void main(void) {
    drawArrow();
    applyBlinnPhong();
}
);


string VRAsphalt::asphaltArrow_dfp_core =
GLSL(
void main(void) {
    drawArrow();
	norm = normalize( gl_NormalMatrix * norm );
    gl_FragData[0] = vec4(position.xyz/position.w, 1.0);
    gl_FragData[1] = vec4(norm, 1);
    gl_FragData[2] = color;
}
);
