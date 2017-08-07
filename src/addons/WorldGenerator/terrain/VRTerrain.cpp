#include "VRTerrain.h"
#include "VRPlanet.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRMaterialT.h"
#include "core/objects/material/VRTexture.h"
#include "core/objects/material/VRTextureGenerator.h"
#include "core/objects/geometry/VRPhysics.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/geometry/OSGGeometry.h"
#include "core/utils/VRFunction.h"
#include "core/math/boundingbox.h"
#include "core/math/polygon.h"
#include "core/math/triangulator.h"
#include "core/math/pose.h"
#include "core/scene/import/GIS/VRGDAL.h"
#include "addons/WorldGenerator/GIS/OSMMap.h"

#include <OpenSG/OSGIntersectAction.h>
#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>

#define GLSL(shader) #shader

using namespace OSG;

VRTerrain::VRTerrain(string name) : VRGeometry(name) { setupMat(); }
VRTerrain::~VRTerrain() {}
VRTerrainPtr VRTerrain::create(string name) { return VRTerrainPtr( new VRTerrain(name) ); }

void VRTerrain::setParameters( Vec2d s, double r, double h ) {
    size = s;
    resolution = r;
    heightScale = h;
    grid = r*64;
    setupGeo();
    mat->setShaderParameter("resolution", resolution);
    mat->setShaderParameter("heightScale", heightScale);
    mat->setShaderParameter("doHeightTextures", 0);
    updateTexelSize();
}

void VRTerrain::setMap( VRTexturePtr t, int channel ) {
    if (!t) return;
    if (t->getChannels() != 4) { // fix mono channels
        VRTextureGenerator tg;
        auto dim = t->getSize();
        tg.setSize(dim, true);
        tex = tg.compose(0);
        for (int i = 0; i < dim[0]; i++) {
            for (int j = 0; j < dim[1]; j++) {
                double h = t->getPixel(Vec3i(i,j,0))[0];
                tex->setPixel(Vec3i(i,j,0), Color4f(1.0,1.0,1.0,h));
            }
        }
    } else tex = t;
    mat->setTexture(tex);
	mat->setShaderParameter("channel", channel);
    updateTexelSize();
}

void VRTerrain::updateTexelSize() {
    if (!tex) return;
    Vec3i s = tex->getSize();
    texelSize[0] = size[0]/s[0];
    texelSize[1] = size[1]/s[1];
	mat->setShaderParameter("texelSize", texelSize);
}

void VRTerrain::setupGeo() {
    Vec2i gridN = Vec2i(size*1.0/grid);
    if (gridN[0] < 1) gridN[0] = 1;
    if (gridN[1] < 1) gridN[1] = 1;
    Vec2d gridS = size;
    gridS[0] /= gridN[0];
    gridS[1] /= gridN[1];
	Vec2d tcChunk = Vec2d(1.0/gridN[0], 1.0/gridN[1]);

	VRGeoData geo;
    for (int i =0; i < gridN[0]; i++) {
		double px1 = -size[0]*0.5 + i*gridS[0];
		double px2 = px1 + gridS[0];
		double tcx1 = 0 + i*tcChunk[0];
		double tcx2 = tcx1 + tcChunk[0];

		for (int j =0; j < gridN[1]; j++) {
			double py1 = -size[1]*0.5 + j*gridS[1];
			double py2 = py1 + gridS[1];
			double tcy1 = 0 + j*tcChunk[1];
			double tcy2 = tcy1 + tcChunk[1];

			geo.pushVert(Vec3d(px1,0,py1), Vec3d(0,1,0), Vec2d(tcx1,tcy1));
			geo.pushVert(Vec3d(px1,0,py2), Vec3d(0,1,0), Vec2d(tcx1,tcy2));
			geo.pushVert(Vec3d(px2,0,py2), Vec3d(0,1,0), Vec2d(tcx2,tcy2));
			geo.pushVert(Vec3d(px2,0,py1), Vec3d(0,1,0), Vec2d(tcx2,tcy1));
			geo.pushQuad();
		}
	}

	geo.apply(ptr());
	setType(GL_PATCHES);
	setPatchVertices(4);
	setMaterial(mat);
}

void VRTerrain::physicalize(bool b) {
    if (!tex) return;
    auto dim = tex->getSize();

    double Hmax = -1e6;
    physicsHeightBuffer = shared_ptr<vector<float>>( new vector<float>(dim[0]*dim[1]) );
    for (int i = 0; i < dim[0]; i++) {
        for (int j = 0; j < dim[1]; j++) {
            int k = j*dim[0]+i;
            float h = tex->getPixel(Vec3i(i,j,0))[3];
            (*physicsHeightBuffer)[k] = h;
            if (Hmax < h) Hmax = h;
        }
    }

    double R = resolution*0.94; // Hack, there is a scaling error somewhere, either in the shape or the visualisation
    auto shape = new btHeightfieldTerrainShape(dim[0], dim[1], &(*physicsHeightBuffer)[0], 1, -Hmax, Hmax, 1, PHY_FLOAT, false);
    shape->setLocalScaling(btVector3(R,1,R));
    getPhysics()->setCustomShape( shape );
    getPhysics()->setPhysicalized(true);
}

void VRTerrain::setSimpleNoise() {
    Color4f w(1,1,1,1);
    VRTextureGenerator tg;
    tg.setSize(Vec3i(128,128,1),true);
    tg.add("Perlin", 1.0, w*0.97, w);
    tg.add("Perlin", 1.0/2, w*0.95, w);
    tg.add("Perlin", 1.0/4, w*0.85, w);
    tg.add("Perlin", 1.0/8, w*0.8, w);
    tg.add("Perlin", 1.0/16, w*0.7, w);
    tg.add("Perlin", 1.0/32, w*0.5, w);
    tex = tg.compose(0);
	auto defaultMat = VRMaterial::get("defaultTerrain");
    defaultMat->setTexture(tex);
}

void VRTerrain::setupMat() {
	auto defaultMat = VRMaterial::get("defaultTerrain");
	tex = defaultMat->getTexture();
	if (!tex) {
        Color4f w(0,0,0,0);
        VRTextureGenerator tg;
        tg.setSize(Vec3i(128,128,1),true);
        tg.drawFill(w);
        tex = tg.compose(0);
        defaultMat->setTexture(tex);
	}

	mat = VRMaterial::create("terrain");
	mat->setWireFrame(0);
	mat->setLineWidth(1);
	mat->setVertexShader(vertexShader, "terrainVS");
	mat->setFragmentShader(fragmentShader, "terrainFS");
	mat->setTessControlShader(tessControlShader, "terrainTCS");
	mat->setTessEvaluationShader(tessEvaluationShader, "terrainTES");
	mat->setShaderParameter("resolution", resolution);
	mat->setShaderParameter("channel", 3);
    updateTexelSize();
	mat->setShaderParameter("texelSize", texelSize);
	mat->setTexture(tex);
}

bool VRTerrain::applyIntersectionAction(Action* action) {
    if (!mesh || !mesh->geo) return false;

    auto tex = getMaterial()->getTexture();

	auto distToSurface = [&](Pnt3d p) -> double {
		float h = getHeight(Vec2d(p[0], p[2]));
		return p[1] - h;
	};

	if (!VRGeometry::applyIntersectionAction(action)) return false;

    IntersectAction* ia = dynamic_cast<IntersectAction*>(action);

    auto ia_line = ia->getLine();
    Pnt3d p0 = Pnt3d(ia_line.getPosition());
    Vec3d d = Vec3d(ia_line.getDirection());
    Pnt3d p = Pnt3d(ia->getHitPoint());

    Vec3f norm(0,1,0); // TODO
    int N = 1000;
    double step = 10; // TODO
    int dir = 1;
    for (int i = 0; i < N; i++) {
        p = p - d*step*dir; // walk
        double l = distToSurface(p);
        if (l > 0 && l < 0.03) {
            Real32 t = p0.dist( p );
            ia->setHit(t, ia->getActNode(), 0, norm, -1);
            break;
        } else if (l*dir > 0) { // jump over surface
            dir *= -1;
            step *= 0.5;
        }
    }

    return true;
}

float VRTerrain::getHeight(const Vec2d& p) { // TODO: trilinear interpolation!
    int W = tex->getSize()[0];
    int H = tex->getSize()[1];
    float u = p[0]/size[0] + 0.5;
    float v = p[1]/size[1] + 0.5;
    int i = floor(u*W);
    int j = floor(v*H);

    float h00 = tex->getPixel(Vec3i(i,j,0))[3];
    float h10 = tex->getPixel(Vec3i(i+1,j,0))[3];
    float h01 = tex->getPixel(Vec3i(i,j+1,0))[3];
    float h11 = tex->getPixel(Vec3i(i+1,j+1,0))[3];

    u = u*W-i;
    v = v*H-j;

    return ( h00*(u) + h10*(1-u) )*v + ( h01*(u) + h11*(1-u) )*(1-v);
}

void VRTerrain::elevateObject(VRTransformPtr t) { auto p = t->getFrom(); elevatePoint(p); t->setFrom(p); }
void VRTerrain::elevatePose(posePtr p) { auto P = p->pos(); elevatePoint(P); p->setPos(P); }
void VRTerrain::elevatePoint(Vec3d& p) { p[1] += getHeight(Vec2d(p[0], p[2])); }

void VRTerrain::loadMap( string path, int channel ) {
    cout << "   ----------- VRTerrain::loadMap " << path << " " << channel << endl ;
    auto tex = loadGeoRasterData(path);
    setMap(tex, channel);
}

void VRTerrain::projectOSM(string path, double N, double E) {
    if (!tex) return;

    // -------------- prepare polygons in terrain space
    Matrix4d terrainMatrix = dynamic_pointer_cast<VRTransform>( getParent() )->getMatrix();
    terrainMatrix.invert();
    map< string, vector<VRPolygonPtr> > polygons;
    auto map = OSMMap::loadMap(path);
    for (auto way : map->getWays()) {
        auto p = way.second->polygon;
        auto pp = VRPolygon::create();

        for (auto pnt : p.get()) {
            Pnt3d pos = Pnt3d( planet->fromLatLongPosition(pnt[1], pnt[0]) );
            terrainMatrix.mult(pos, pos);
            pp->addPoint( Vec2d(pos[0], pos[2]) );
        }

        for (auto tag : way.second->tags) polygons[tag.first].push_back(pp);
    }

    // training ground hack flat ground
    /*auto tgPolygon = VRPolygon::create();
    Pnt3d pos;
    float d = 0.003;
    pos = Pnt3d( planet->fromLatLongPosition(29.924500-d, 119.896806-d) );
    terrainMatrix.mult(pos, pos); tgPolygon->addPoint( Vec2d(pos[0], pos[2]) );
    pos = Pnt3d( planet->fromLatLongPosition(29.924500-d, 119.896806+d) );
    terrainMatrix.mult(pos, pos); tgPolygon->addPoint( Vec2d(pos[0], pos[2]) );
    pos = Pnt3d( planet->fromLatLongPosition(29.924500+d, 119.896806+d) );
    terrainMatrix.mult(pos, pos); tgPolygon->addPoint( Vec2d(pos[0], pos[2]) );
    pos = Pnt3d( planet->fromLatLongPosition(29.924500+d, 119.896806-d) );
    terrainMatrix.mult(pos, pos); tgPolygon->addPoint( Vec2d(pos[0], pos[2]) );
    tgPolygon->scale( Vec3d(1.0/size[0], 1, 1.0/size[1]) );
    tgPolygon->translate( Vec3d(0.5,0,0.5) );*/

    // -------------------- project OSM polygons on texture
    auto dim = tex->getSize();
    VRTextureGenerator tg;
    tg.setSize(dim, true);

    //for (auto tag : polygons) cout << "polygon tag: " << tag.first << endl;

    auto drawPolygons = [&](string tag, Color4f col) {
        if (!polygons.count(tag)) {
            //cout << "\ndrawPolygons: tag '" << tag << "' not found!" << endl;
            return;
        }

        for (auto p : polygons[tag]) {
            p->scale( Vec3d(1.0/size[0], 1, 1.0/size[1]) );
            p->translate( Vec3d(0.5,0,0.5) );
            tg.drawPolygon( p, col );
        }
    };

    drawPolygons("natural", Color4f(0,1,0,1));
    drawPolygons("water", Color4f(0.2,0.4,1,1));
    drawPolygons("industrial", Color4f(0.2,0.2,0.2,1));
    VRTexturePtr t = tg.compose(0);

    // ----------------------- combine OSM texture with heightmap
    for (int i = 0; i < dim[0]; i++) {
        for (int j = 0; j < dim[1]; j++) {
            Vec3i pixK = Vec3i(i,j,0);
            double h = tex->getPixel(pixK)[3];
            auto pix = Vec2d(i*1.0/(dim[0]-1), j*1.0/(dim[1]-1));
            //if (tgPolygon->isInside(pix)) h = 14;
            Color4f col = t->getPixel(pixK);
            col[3] = h;
            t->setPixel(pixK, col);
        }
    }
    setMap(t);
}

void VRTerrain::setPlanet(VRPlanetPtr p, Vec2d position) {
    sphericalCoordinates = position;
    planet = p;
}

void VRTerrain::paintHeights(string path) {
    mat->setTexture(path, 0, 1);
    mat->setTexture("world/textures/gravel2.jpg", 0, 2);
    mat->setShaderParameter("texWoods", 1);
    mat->setShaderParameter("texGravel", 2);
    mat->setShaderParameter("doHeightTextures", 1);
}



// --------------------------------- shader ------------------------------------

string VRTerrain::vertexShader =
"#version 120\n"
GLSL(
attribute vec4 osg_Vertex;
attribute vec2 osg_MultiTexCoord0;

void main(void) {
    gl_TexCoord[0] = vec4(osg_MultiTexCoord0,0.0,0.0);
	gl_Position = osg_Vertex;
}
);

string VRTerrain::fragmentShader =
"#version 400 compatibility\n"
GLSL(
uniform sampler2D tex;
uniform sampler2D texWoods;
uniform sampler2D texGravel;
const ivec3 off = ivec3(-1,0,1);
const vec3 light = vec3(-1,-1,-0.5);
uniform vec2 texelSize;
uniform int doHeightTextures;

in float height;
vec3 norm;
vec4 color;

vec3 mixColor(vec3 c1, vec3 c2, float t) {
	t = clamp(t, 0.0, 1.0);
	return mix(c1, c2, t);
}

vec3 getColor() {
	return texture2D(tex, gl_TexCoord[0].xy).rgb;
}

void applyBlinnPhong() {
	norm = normalize( gl_NormalMatrix * norm );
	vec3  light = normalize( gl_LightSource[0].position.xyz );// directional light
	float NdotL = max(dot( norm, light ), 0.0);
	vec4  ambient = gl_LightSource[0].ambient * color;
	vec4  diffuse = gl_LightSource[0].diffuse * NdotL * color;
	float NdotHV = max(dot( norm, normalize(gl_LightSource[0].halfVector.xyz)),0.0);
	vec4  specular = gl_LightSource[0].specular * pow( NdotHV, gl_FrontMaterial.shininess );
	//gl_FragColor = ambient + diffuse + specular;
	gl_FragColor = diffuse + specular;
	gl_FragColor[3] = 1.0;
	//gl_FragColor = vec4(diffuse.rgb, 1);
}

vec3 getNormal() {
	vec2 tc = gl_TexCoord[0].xy;
    float s11 = texture(tex, tc).a;
    float s01 = textureOffset(tex, tc, off.xy).a;
    float s21 = textureOffset(tex, tc, off.zy).a;
    float s10 = textureOffset(tex, tc, off.yx).a;
    float s12 = textureOffset(tex, tc, off.yz).a;

    vec2 r2 = 2.0*texelSize;
    vec3 va = normalize(vec3(r2.x,s21-s01,0));
    vec3 vb = normalize(vec3(   0,s12-s10,r2.y));
    vec3 n = cross(vb,va);
	return n;
}

void main( void ) {
	vec2 tc = gl_TexCoord[0].xy;
	norm = getNormal();

	if (doHeightTextures == 0) color = vec4(getColor(),1.0);
	else {
        vec4 cW1 = texture(texWoods, tc*1077);
        vec4 cW2 = texture(texWoods, tc*107);
        vec4 cW3 = texture(texWoods, tc*17);
        vec4 cW4 = texture(texWoods, tc);
        vec4 cW = mix(cW1,mix(cW2,mix(cW3,cW4,0.5),0.5),0.5);

        vec4 cG0 = texture(texGravel, tc*10777);
        vec4 cG1 = texture(texGravel, tc*1077);
        vec4 cG2 = texture(texGravel, tc*107);
        vec4 cG3 = texture(texGravel, tc*17);
        vec4 cG4 = texture(texGravel, tc);
        vec4 cG = mix(cG0,mix(cG1,mix(cG2,mix(cG3,cG4,0.5),0.5),0.5),0.5);
        color = mix(cG, cW, min(cW3.r*0.1*max(height,0),1));
	}

	applyBlinnPhong();
}
);

string VRTerrain::tessControlShader =
"#version 400 compatibility\n"
"#extension GL_ARB_tessellation_shader : enable\n"
GLSL(
layout(vertices = 4) out;
out vec3 tcPosition[];
out vec2 tcTexCoords[];
uniform float resolution;
)
"\n#define ID gl_InvocationID\n"
GLSL(
void main() {
    tcPosition[ID] = gl_in[ID].gl_Position.xyz;
    tcTexCoords[ID] = gl_in[ID].gl_TexCoord[0].xy;

    if (ID == 0) {
		vec4 mid = (gl_in[0].gl_Position + gl_in[1].gl_Position + gl_in[2].gl_Position + gl_in[3].gl_Position) * 0.25;
		vec4 pos = gl_ModelViewProjectionMatrix * vec4(mid.xyz, 1.0);
		float D = length(pos.xyz);
		//int p = int(5.0/(resolution*D));
		//int res = int(pow(2,p));
		int res = int(resolution*2048/D); // 32*64
		res = clamp(res, 1, 64);
		if (res >= 64) res = 64; // take closest power of two to avoid jumpy effects
		else if (res >= 32) res = 32;
		else if (res >= 16) res = 16;
		else if (res >= 8) res = 8;
		else if (res >= 4) res = 4;
		else if (res >= 2) res = 2;

        gl_TessLevelInner[0] = res;
        gl_TessLevelInner[1] = res;

        gl_TessLevelOuter[0] = 64;
        gl_TessLevelOuter[1] = 64;
        gl_TessLevelOuter[2] = 64;
        gl_TessLevelOuter[3] = 64;
    }
}
);

string VRTerrain::tessEvaluationShader =
"#version 400 compatibility\n"
"#extension GL_ARB_tessellation_shader : enable\n"
GLSL(
layout( quads ) in;
in vec3 tcPosition[];
in vec2 tcTexCoords[];
out float height;

uniform float heightScale;
uniform int channel;
uniform sampler2D texture;

void main() {
    float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;

    vec2 ta = mix(tcTexCoords[0], tcTexCoords[1], u);
    vec2 tb = mix(tcTexCoords[3], tcTexCoords[2], u);
    vec2 tc = mix(ta, tb, v);
    gl_TexCoord[0] = vec4(tc.x, tc.y, 1.0, 1.0);

    vec3 a = mix(tcPosition[0], tcPosition[1], u);
    vec3 b = mix(tcPosition[3], tcPosition[2], u);
    vec3 tePosition = mix(a, b, v);
    height = heightScale * texture2D(texture, gl_TexCoord[0].xy)[channel];
    tePosition.y = height;
    gl_Position = gl_ModelViewProjectionMatrix * vec4(tePosition, 1);
}
);

