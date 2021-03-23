#include "VRTerrain.h"
#include "VRPlanet.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRMaterialT.h"
#include "core/objects/material/VRTexture.h"
#include "core/objects/material/VRTextureGenerator.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/geometry/OSGGeometry.h"
#include "core/utils/VRFunction.h"
#include "core/math/boundingbox.h"
#include "core/math/polygon.h"
#include "core/math/triangulator.h"
#include "core/math/pose.h"
#include "core/math/path.h"
#include "core/scene/import/GIS/VRGDAL.h"
#include "core/scene/VRScene.h"
#include "addons/WorldGenerator/GIS/OSMMap.h"

#include <OpenSG/OSGIntersectAction.h>
#include <OpenSG/OSGGeoProperties.h>
#include <OpenSG/OSGGeometry.h>
#include <boost/thread/recursive_mutex.hpp>

#ifndef WITHOUT_BULLET
#include "core/objects/geometry/VRPhysics.h"
#include "VRTerrainPhysicsShape.h"
#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>
#endif

typedef boost::recursive_mutex::scoped_lock PLock;

#define GLSL(shader) #shader

using namespace OSG;


VREmbankment::VREmbankment(PathPtr p1, PathPtr p2, PathPtr p3, PathPtr p4) : VRGeometry("embankment"), p1(p1), p2(p2), p3(p3), p4(p4) {
    for (auto p : p1->getPoints()) { auto pos = p.pos(); area.addPoint(Vec2d(pos[0],pos[2])); };
    for (auto p : p2->getPoints()) { auto pos = p.pos(); area.addPoint(Vec2d(pos[0],pos[2])); };
    hide("SHADOW");
}

VREmbankment::~VREmbankment() {}
VREmbankmentPtr VREmbankment::create(PathPtr p1, PathPtr p2, PathPtr p3, PathPtr p4) { return VREmbankmentPtr( new VREmbankment(p1,p2,p3,p4) ); }

bool VREmbankment::isInside(Vec2d p) { return area.isInside(p); }

float VREmbankment::getHeight(Vec2d p) { // TODO: optimize!
    auto computeHeight = [&](float h) {
        Vec3d P(p[0], h, p[1]);
        float t1 = p1->getClosestPoint(P);
        float t2 = p2->getClosestPoint(P);
        Vec3d P1 = p1->getPosition(t1);
        Vec3d P2 = p2->getPosition(t2);
        float d1 = (P1-P).length();
        float d2 = (P2-P).length();
        float t = d2/(d1+d2);
        return P1[1]*t+P2[1]*(1-t);
    };

    float h1 = computeHeight(0); // first estimate
    float h2 = computeHeight(h1); // first estimate
    float h3 = computeHeight(h2); // first estimate
    return h3;
}

vector<Vec3d> VREmbankment::probeHeight(Vec2d p) { // TODO: optimize!
    vector<Vec3d> res(3);

    auto computeHeight = [&](float h) {
        Vec3d P(p[0], h, p[1]);
        float t1 = p1->getClosestPoint(P);
        float t2 = p2->getClosestPoint(P);
        Vec3d P1 = p1->getPosition(t1);
        Vec3d P2 = p2->getPosition(t2);
        float d1 = (P1-P).length();
        float d2 = (P2-P).length();
        float t = d2/(d1+d2);
        float H = P1[1]*t+P2[1]*(1-t);

        res[0] = Vec3d(p[0], H, p[1]);
        res[1] = P1;
        res[2] = P2;
        return H;
    };

    float h1 = computeHeight(0); // first estimate
    float h2 = computeHeight(h1); // first estimate
    computeHeight(h2); // first estimate
    getMaterial()->setWireFrame(1);
    return res;
}

void VREmbankment::createGeometry() {
    float res = 0.025;
    int N = round(1.0/res);
    VRGeoData data;
    Vec3d u = Vec3d(0,1,0);
    for (int i=0; i<=N; i++) {
        auto pos1 = p1->getPosition(i*res);
        auto pos2 = p2->getPosition(1-i*res);
        auto pos3 = p3->getPosition(i*res);
        auto pos4 = p4->getPosition(1-i*res);
        data.pushVert(pos1, u, Vec2d(pos1[0], pos1[2]));
        data.pushVert(pos2, u, Vec2d(pos2[0], pos2[2]));
        data.pushVert(pos3, u, Vec2d(pos3[0], pos3[2]));
        data.pushVert(pos4, u, Vec2d(pos4[0], pos4[2]));
        if (i < N) {
            data.pushQuad(i*4, i*4+1, (i+1)*4+1, (i+1)*4); // top
            data.pushQuad(i*4, (i+1)*4, (i+1)*4+2, i*4+2); // side1
            data.pushQuad(i*4+1, i*4+3, (i+1)*4+3, (i+1)*4+1); // side2
        }
    }

    data.apply(ptr());
    updateNormals();
}


VRTerrain::VRTerrain(string name, bool localized) : VRGeometry(name) {
    hide("SHADOW");
    localMesh = localized;
}

VRTerrain::~VRTerrain() {}

VRTerrainPtr VRTerrain::create(string name, bool localized) {
    auto t = VRTerrainPtr( new VRTerrain(name, localized) );
    t->setupMat();
    return t;
}

VRTerrainPtr VRTerrain::ptr() { return dynamic_pointer_cast<VRTerrain>( shared_from_this() ); }

void VRTerrain::clear() {
    for (auto e : embankments) e.second->destroy();
    embankments.clear();
}

void VRTerrain::setParameters( Vec2d s, double r, double h, float w, float aT, Color3f aC, bool isLit) {
    size = s;
    resolution = r;
    heightScale = h;
#ifdef __EMSCRIPTEN__
    grid = r;
#else
    grid = r*64;
#endif
    mat->setShaderParameter("resolution", resolution);
    mat->setShaderParameter("heightScale", heightScale);
    mat->setShaderParameter("doHeightTextures", 0);
    mat->setShaderParameter("waterLevel", w);
    mat->setShaderParameter("isLit", int(isLit));
    mat->setShaderParameter("atmoColor", aC);
    mat->setShaderParameter("atmoThickness", aT);
    updateTexelSize();
    setupGeo();
}

void VRTerrain::setLocalized(bool in){ localMesh = in; }
void VRTerrain::setLODFactor(double in){ LODfac = in; }
double VRTerrain::getLODFactor(){ return LODfac; }
void VRTerrain::setMeshTer(vector<vector<vector<Vec3d>>> in){ meshTer = in; }
void VRTerrain::setWaterLevel(float w) { mat->setShaderParameter("waterLevel", w); }
void VRTerrain::setLit(bool isLit) { mat->setShaderParameter("isLit", int(isLit)); }
void VRTerrain::setAtmosphericEffect(float thickness, Color3f color) { mat->setShaderParameter("atmoColor", color); mat->setShaderParameter("atmoThickness", thickness); }
void VRTerrain::setHeightScale(float s) { heightScale = s; mat->setShaderParameter("heightScale", s); }

void VRTerrain::setMap( VRTexturePtr t, int channel ) {
    PLock lock(mtx());
    if (!t) return;
    if (t->getChannels() != 4) { // fix mono channels
        VRTextureGenerator tg;
        auto dim = t->getSize();
        tg.setSize(dim, true);
        heigthsTex = tg.compose(0);
        for (int i = 0; i < dim[0]; i++) {
            for (int j = 0; j < dim[1]; j++) {
                double h = t->getPixelVec(Vec3i(i,j,0))[0];
                heigthsTex->setPixel(Vec3i(i,j,0), Color4f(1.0,1.0,1.0,h));
            }
        }
    } else heigthsTex = t;
    mat->setTexture(heigthsTex);
    mat->clearTransparency();
	mat->setShaderParameter("channel", channel);
	mat->setShaderParameter("heightoffset", heightoffset);
    mat->setTextureParams(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_MODULATE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
    mat->clearTransparency();
    updateTexelSize();
    setupGeo();
}

void VRTerrain::paintHeights(string woods, string gravel) {
    mat->setTexture(woods, 0, 1);
    mat->setTexture(gravel, 0, 2);
    mat->setShaderParameter("texWoods", 1);
    mat->setShaderParameter("texGravel", 2);
    mat->setShaderParameter("doHeightTextures", 1);
    mat->clearTransparency();
}

void VRTerrain::paintHeights(string path, Color4f mCol, float mAmount) {
    mat->setTexture(path, 0, 3);
    //if (mAmount > 0 )
    //    if (auto t = mat->getTexture(3)) t->mixColor(mCol, mAmount);
    mat->setShaderParameter("mixColor", mCol);
    mat->setShaderParameter("mixAmount", mAmount);
    mat->setShaderParameter("texPic", 3);
    mat->setShaderParameter("doHeightTextures", 2);
    mat->clearTransparency();
}

void VRTerrain::updateTexelSize() {
    if (!heigthsTex) return;
    Vec3i s = heigthsTex->getSize();
    Vec2f texel = Vec2f(1.0/(s[0]-1), 1.0/(s[1]-1));
    texelSize[0] = size[0]*texel[0];
    texelSize[1] = size[1]*texel[1];
	mat->setShaderParameter("texel", texel);
	mat->setShaderParameter("texelSize", texelSize);
}

void VRTerrain::curveMesh(VRPlanetPtr p, Vec2d c, PosePtr s) {
    localMesh = true;
    planet = p;
    planetCoords = c;
    pSectorInv = s;
}

void VRTerrain::setupGeo() {
    Vec2i gridN = Vec2i(round(size[0]*1.0/grid-0.5), round(size[1]*1.0/grid-0.5));
    if (gridN[0] < 1) gridN[0] = 1;
    if (gridN[1] < 1) gridN[1] = 1;
    Vec2d gridS = size;
    gridS[0] /= gridN[0];
    gridS[1] /= gridN[1];

    if (!heigthsTex) { cout << "VRTerrain::setupGeo -- no heigthsTex loaded" << endl; return; }
    auto texSize = heigthsTex->getSize();
    Vec2d texel = Vec2d( 1.0/texSize[0], 1.0/texSize[1] );
	Vec2d tcChunk = Vec2d((1.0-texel[0])/gridN[0], (1.0-texel[1])/gridN[1]);
    //cout << toString(gridN) << "-- "  << toString(size) << "-- "  << toString(texel) << "-- "  << toString(tcChunk) << "-- "  << toString(texSize) << endl;

	VRGeoData geo;
    auto pla = planet.lock();
	if (localMesh && pla) {
        mat->setShaderParameter("local",1);
        double sectorSize = pla->getSectorSize();

        int t1 = 0;
        int t2 = 0;

        map<Vec2i, PosePtr> cache;

        auto getPose = [&](int i, int j) {
            if (cache.count(Vec2i(i,j))) return cache[Vec2i(i,j)];
            double N = planetCoords[0]+sectorSize*(1.0-double(j)/double(gridN[1]));
            double E = planetCoords[1]+i*sectorSize/gridN[0];
            auto p = pla->fromLatLongPose(N, E);
            p = pSectorInv->multRight(p);
            cache[Vec2i(i,j)] = p;
            //if (i == 0) pla->addPin( "", N, E, 1000);
            //cout << "  VRTerrain::setupGeo add pin " << N << " " << E << " " << pla->getName() << endl;
            return p;
        };

        for (int i =0; i < gridN[0]; i++) {
            t1++;
            t2 = 0;
            double tcx1 = texel[0]*0.5 + i*tcChunk[0];
            double tcx2 = tcx1 + tcChunk[0];
            for (int j =0; j < gridN[1]; j++) {
                t2++;
                double tcy1 = texel[1]*0.5 + j*tcChunk[1];
                double tcy2 = tcy1 + tcChunk[1];

                auto pI0J0 = getPose(i,j);
                auto pI0J1 = getPose(i,j+1);
                auto pI1J1 = getPose(i+1,j+1);
                auto pI1J0 = getPose(i+1,j);

                geo.pushVert(pI0J0->pos(), pI0J0->up(), Vec2d(tcx1,tcy1));
                geo.pushVert(pI0J1->pos(), pI0J1->up(), Vec2d(tcx1,tcy2));
                geo.pushVert(pI1J1->pos(), pI1J1->up(), Vec2d(tcx2,tcy2));
                geo.pushVert(pI1J0->pos(), pI1J0->up(), Vec2d(tcx2,tcy1));
                geo.pushQuad();
            }
        }
	} else if (!localMesh) {
        int old1 = 0;
        int old2 = 0;
        for (int i =0; i < gridN[0]; i++) {
            old1++;
            old2 = 0;
            double px1 = -size[0]*0.5 + i*gridS[0];
            double px2 = px1 + gridS[0];
            double tcx1 = texel[0]*0.5 + i*tcChunk[0];
            double tcx2 = tcx1 + tcChunk[0];

            for (int j =0; j < gridN[1]; j++) {
                old2++;
                double py1 = -size[1]*0.5 + j*gridS[1];
                double py2 = py1 + gridS[1];
                double tcy1 = texel[1]*0.5 + j*tcChunk[1];
                double tcy2 = tcy1 + tcChunk[1];
                geo.pushVert(Vec3d(px1,0,py1), Vec3d(0,1,0), Vec2d(tcx1,tcy1));
                geo.pushVert(Vec3d(px1,0,py2), Vec3d(0,1,0), Vec2d(tcx1,tcy2));
                geo.pushVert(Vec3d(px2,0,py2), Vec3d(0,1,0), Vec2d(tcx2,tcy2));
                geo.pushVert(Vec3d(px2,0,py1), Vec3d(0,1,0), Vec2d(tcx2,tcy1));
                geo.pushQuad();
            }
        }
	}

    if (geo.size() > 0) {
        //cout << "  VRTerrain::setupGeo apply geo!! " << geo.size() << "  grid: " << gridN << endl;
        geo.apply(ptr());
    }

#if __EMSCRIPTEN__// TODO: directly create triangles above!
    convertToTriangles();
#else
	setType(GL_PATCHES);
	setPatchVertices(4);
#endif
	setMaterial(mat);
}

vector<Vec3d> VRTerrain::probeHeight( Vec2d p ) {
    Vec2d uv = toUVSpace(p); // uv, i and j are tested
    int i = round(uv[0]-0.5);
    int j = round(uv[1]-0.5);

    double h00 = heigthsTex->getPixelVec(Vec3i(i,j,0))[3];
    double h10 = heigthsTex->getPixelVec(Vec3i(i+1,j,0))[3];
    double h01 = heigthsTex->getPixelVec(Vec3i(i,j+1,0))[3];
    double h11 = heigthsTex->getPixelVec(Vec3i(i+1,j+1,0))[3];

    double u = uv[0]-i;
    double v = uv[1]-j;
    double h = ( h00*(1-u) + h10*u )*(1-v) + ( h01*(1-u) + h11*u )*v;

    Vec2d p0 = fromUVSpace( Vec2d(i,j) );
    Vec2d p1 = fromUVSpace( Vec2d(i+1,j+1) );
    //cout << " VRTerrain::getHeight " << uv << " " << i << " " << j << " " << W << " " << H << endl;
    for (auto e : embankments) if (e.second->isInside(p)) return e.second->probeHeight(p);

    return {Vec3d(p[0], h, p[1]),
            Vec3d(p0[0], h00, p0[1]),
            Vec3d(p1[0], h10, p0[1]),
            Vec3d(p0[0], h01, p1[1]),
            Vec3d(p1[0], h11, p1[1]) };
}

VRTexturePtr VRTerrain::getMap() { return heigthsTex; }
Vec2f VRTerrain::getTexelSize() { return texelSize; }

void VRTerrain::btPhysicalize() {
    auto dim = heigthsTex->getSize();
    float roadTerrainOffset = 0.03; // also defined in vrroadbase.cpp

    double Hmax = -1e6;
    physicsHeightBuffer = shared_ptr<vector<float>>( new vector<float>(dim[0]*dim[1]) );
    for (int i = 0; i < dim[0]; i++) {
        for (int j = 0; j < dim[1]; j++) {
            int k = j*dim[0]+i;
            float h = heigthsTex->getPixelVec(Vec3i(i,j,0))[3];
            (*physicsHeightBuffer)[k] = h + roadTerrainOffset;
            if (Hmax < h) Hmax = h;
        }
    }
#ifndef WITHOUT_BULLET
    auto shape = new btHeightfieldTerrainShape(dim[0], dim[1], &(*physicsHeightBuffer)[0], 1, -Hmax, Hmax, 1, PHY_FLOAT, false);
    shape->setLocalScaling(btVector3(texelSize[0],1,texelSize[1]));
    getPhysics()->setCustomShape( shape );
#endif
}

void VRTerrain::vrPhysicalize() {
#ifndef WITHOUT_BULLET
    auto shape = new VRTerrainPhysicsShape( ptr(), resolution );
    getPhysics()->setCustomShape( shape );
#endif
}

void VRTerrain::physicalize(bool b) {
#ifndef WITHOUT_BULLET
    if (!heigthsTex) return;
    if (!b) { getPhysics()->setPhysicalized(false); return; }

    //btPhysicalize();
    vrPhysicalize();
    getPhysics()->setFriction(0.8);
    getPhysics()->setPhysicalized(true);
#endif
}

Boundingbox VRTerrain::getBoundingBox() {
    Boundingbox bb;
    float hmax = -1e30;
    float hmin = 1e30;

    for (int i=0; i<heigthsTex->getSize()[0]; i++) {
        for (int j=0; j<heigthsTex->getSize()[1]; j++) {
            auto h = heigthsTex->getPixelVec(Vec3i(i,j,0))[3];
            if (h < hmin) hmin = h;
            if (h > hmax) hmax = h;
        }
    }

    bb.update( Vec3d( size[0]*0.5, hmax,  size[1]*0.5) );
    bb.update( Vec3d(-size[0]*0.5, hmin, -size[1]*0.5) );
    return bb;
}

void VRTerrain::setSimpleNoise() {
    PLock lock(mtx());
    Color4f w(1,1,1,1);
    VRTextureGenerator tg;
    tg.setSize(Vec3i(128,128,1),true);
    tg.add("Perlin", 1.0, w*0.97, w);
    tg.add("Perlin", 1.0/2, w*0.95, w);
    tg.add("Perlin", 1.0/4, w*0.85, w);
    tg.add("Perlin", 1.0/8, w*0.8, w);
    tg.add("Perlin", 1.0/16, w*0.7, w);
    tg.add("Perlin", 1.0/32, w*0.5, w);
    heigthsTex = tg.compose(0);
	auto defaultMat = VRMaterial::get("defaultTerrain");
    defaultMat->setTexture(heigthsTex);
    defaultMat->clearTransparency();
}

boost::recursive_mutex& VRTerrain::mtx() {
#ifndef WITHOUT_BULLET
    auto scene = VRScene::getCurrent();
    if (scene) return scene->physicsMutex();
    else
#endif
    {
        static boost::recursive_mutex m;
        return m;
    };
}

void VRTerrain::setHeightTexture(VRTexturePtr t) {
    PLock lock(mtx());
    heigthsTex = t;
}

void VRTerrain::setupMat() {
	auto defaultMat = VRMaterial::get("defaultTerrain");
	setHeightTexture(defaultMat->getTexture());
    Vec2f texel = Vec2f(1,1);
	if (!heigthsTex) {
        Color4f w(0,0,0,0);
        VRTextureGenerator tg;
        tg.setSize(Vec3i(128,128,1),true);
        tg.drawFill(w);
        setHeightTexture( tg.compose(0) );
        defaultMat->setTexture(heigthsTex);
        defaultMat->clearTransparency();
        auto texSize = heigthsTex->getSize();
        texel = Vec2f( 1.0/texSize[0], 1.0/texSize[1] );
	}

	mat = VRMaterial::create("terrain");
#ifndef OSG_OGL_ES2
	mat->setVertexShader(vertexShader, "terrainVS");
	mat->setFragmentShader(fragmentShader, "terrainFS");
	mat->setFragmentShader(fragmentShaderDeferred, "terrainFSD", true);
	mat->setTessControlShader(tessControlShader, "terrainTCS");
	mat->setTessEvaluationShader(tessEvaluationShader, "terrainTES");
#else
    mat->setVertexShader(vertexShader_es2, "terrainVSes2");
    mat->setFragmentShader(fragmentShader_es2, "terrainFSes2");
#endif
	mat->setShaderParameter("resolution", resolution);
	mat->setShaderParameter("channel", 3);
	mat->setShaderParameter("texel", texel);
	mat->setShaderParameter("texelSize", texelSize);
    mat->setZOffset(1,1);
	setMap(heigthsTex);
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
    double step = 10;
    int dir = 1;
    for (int i = 0; i < N; i++) {
        p = p - d*step*dir; // walk
        double l = distToSurface(p);
        if (i == 0) step = abs(l);
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

Vec2d VRTerrain::toUVSpace(Vec2d p) {
    int W = heigthsTex->getSize()[0]-1;
    int H = heigthsTex->getSize()[1]-1;
    double u = (p[0]/size[0] + 0.5)*W;
    double v = (p[1]/size[1] + 0.5)*H;
    return Vec2d(u,v);
};

Vec2d VRTerrain::fromUVSpace(Vec2d uv) {
    int W = heigthsTex->getSize()[0]-1;
    int H = heigthsTex->getSize()[1]-1;
    double x = ((uv[0])/W-0.5)*size[0];
    double z = ((uv[1])/H-0.5)*size[1];
    return Vec2d(x,z);
};

Vec2d VRTerrain::getTexCoord( Vec2d p ) {
    double u = p[0]/size[0] + 0.5;
    double v = p[1]/size[1] + 0.5;
    return Vec2d(u,v);
}

double VRTerrain::getHeight(Vec2d p, bool useEmbankments) {
    Vec2d uv = toUVSpace(p); // uv, i and j are tested
    int i = round(uv[0]-0.5);
    int j = round(uv[1]-0.5);

    double h00 = heigthsTex->getPixelVec(Vec3i(i,j,0))[3];
    double h10 = heigthsTex->getPixelVec(Vec3i(i+1,j,0))[3];
    double h01 = heigthsTex->getPixelVec(Vec3i(i,j+1,0))[3];
    double h11 = heigthsTex->getPixelVec(Vec3i(i+1,j+1,0))[3];

    double u = uv[0]-i;
    double v = uv[1]-j;

    double h = ( h00*(1-u) + h10*u )*(1-v) + ( h01*(1-u) + h11*u )*v;
    if (useEmbankments) {
        for (auto e : embankments) {
            if (e.second->isInside(p)) {
                double k = e.second->getHeight(p);
                if (k > h) h = k;
            }
        }
    }
    return h;
}

void VRTerrain::elevateObject(VRTransformPtr t, float offset) { auto p = t->getFrom(); p = elevatePoint(p, offset); t->setFrom(p); }
void VRTerrain::elevatePose(PosePtr p, float offset) { auto P = p->pos(); P = elevatePoint(P, offset); p->setPos(P); }
Vec3d VRTerrain::elevatePoint(Vec3d p, float offset, bool useEmbankments) { p[1] = getHeight(Vec2d(p[0], p[2]), useEmbankments) + offset;  return p; }

void VRTerrain::elevateVertices(VRGeometryPtr geo, float offset) {
    auto t = terrain.lock();
    if (!t || !geo || !geo->getMesh() || !geo->getMesh()->geo) return;
    GeoPnt3fPropertyMTRecPtr pos = (GeoPnt3fProperty*)geo->getMesh()->geo->getPositions();
    for (unsigned int i=0; i<pos->size(); i++) {
        Pnt3f p;
        pos->getValue(p, i);
        p[1] = getHeight(Vec2d(p[0], p[2])) + offset;
        pos->setValue(p, i);
    }
}

void VRTerrain::elevatePolygon(VRPolygonPtr poly, float offset, bool useEmbankments) {
    for (auto p2 : poly->get()) {
        Vec3d p3(p2[0], 0, p2[1]);
        p3 = elevatePoint(p3, offset, useEmbankments);
        poly->addPoint(p3);
    }
    poly->get().clear();
}

void VRTerrain::projectTangent( Vec3d& t, Vec3d p) {
    t[1] = 0;
    t.normalize();
    t *= resolution;
    float h1 = getHeight(Vec2d(p[0]-t[0], p[2]-t[2]));
    float h2 = getHeight(Vec2d(p[0]+t[0], p[2]+t[2]));
    t[1] = h2-h1;
    t.normalize();
}

Vec3d VRTerrain::getNormal( Vec3d p ) { // TODO!!
    Vec3d ex(1,0,0);
    Vec3d ez(0,0,1);
    projectTangent(ex, p);
    projectTangent(ez, p);
    Vec3d n = ez.cross(ex);
    n.normalize();
    return n;
}

void VRTerrain::loadMap( string path, int channel, bool shout ) {
#ifndef WITHOUT_GDAL
    if (shout) cout << "   ----------- VRTerrain::loadMap " << path << " " << channel << endl ;
    if (useHeightoffset) {
        heightoffset = 0.0;
        auto tex = loadGeoRasterData(path, shout, &heightoffset);
        setMap(tex, channel);
    } else {
        auto tex = loadGeoRasterData(path, shout);
        setMap(tex, channel);
    }
#endif
}

void VRTerrain::setHeightOffset( bool enab ) {
#ifndef WITHOUT_GDAL
    useHeightoffset = enab;
#endif
}

double VRTerrain::getHeightOffset() {
    return heightoffset;
}

void VRTerrain::flatten(vector<Vec2d> perimeter, float h) {
    if (!heigthsTex) return;
    PLock lock(mtx());
    VRPolygonPtr poly = VRPolygon::create();
    for (auto p : perimeter) poly->addPoint(p);
    poly->scale( Vec3d(1.0/size[0], 1, 1.0/size[1]) );
    poly->translate( Vec3d(0.5,0,0.5) );

    auto dim = heigthsTex->getSize();
    for (int i = 0; i < dim[0]; i++) {
        for (int j = 0; j < dim[1]; j++) {
            auto pix = Vec2d(i*1.0/(dim[0]-1), j*1.0/(dim[1]-1));
            if (poly->isInside(pix)) {
                Vec3i pixK = Vec3i(i,j,0);
                Color4f col = heigthsTex->getPixelVec(pixK);
                col[3] = h;
                heigthsTex->setPixel(pixK, col);
            }
        }
    }
    setMap(heigthsTex);
}

void VRTerrain::projectOSM() {
    /*if (!tex) return;

    // -------------- prepare polygons in terrain space
    Matrix4d terrainMatrix = dynamic_pointer_cast<VRTransform>( getParent() )->getMatrix();
    terrainMatrix.invert();
    map< string, vector<VRPolygonPtr> > polygons;
    auto map = world->get;
    for (auto way : map->getWays()) {
        auto p = way.second->polygon;
        auto pp = VRPolygon::create();

        for (auto pnt : p.get()) {
            Pnt3d pos = Pnt3d( planet->fromLatLongPosition(pnt[1], pnt[0]) );
            terrainMatrix.mult(pos, pos);
            pp->addPoint( Vec2d(pos[0], pos[2]) );
        }

        for (auto tag : way.second->tags) polygons[tag.first].push_back(pp);
    }*/

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
    /*auto dim = tex->getSize();
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
    setMap(t);*/
}

void VRTerrain::addEmbankment(string ID, PathPtr p1, PathPtr p2, PathPtr p3, PathPtr p4) {
    auto e = VREmbankment::create(p1, p2, p3, p4);
    auto m = VRMaterial::get("embankment");
    m->setTexture("world/textures/gravel2.jpg");
    m->setDiffuse(Color3f(0.5,0.5,0.5));
    m->setZOffset(1,1);
    e->createGeometry();
    e->setMaterial(m);
    addChild(e);
    embankments[ID] = e;
}

Vec2d VRTerrain::getSize() { return size; }
double VRTerrain::getGrid() { return grid; }

// --------------------------------- shader ------------------------------------

string VRTerrain::vertexShader =
"#version 120\n"
GLSL(
attribute vec4 osg_Vertex;
attribute vec3 osg_Normal;
attribute vec2 osg_MultiTexCoord0;
varying vec3 vNormal;

void main(void) {
    gl_TexCoord[0] = vec4(osg_MultiTexCoord0,0.0,0.0);
	gl_Position = osg_Vertex;
	vNormal = osg_Normal;
}
);

string VRTerrain::vertexShader_es2 =
GLSL(
attribute vec4 osg_Vertex;
attribute vec3 osg_Normal;
attribute vec2 osg_MultiTexCoord0;

varying vec3 vNormal;
varying vec4 vColor;
varying vec4 vVertex;
varying vec2 vTexCoord;

uniform sampler2D tex;
uniform float heightScale;
uniform int local;
uniform int channel;
uniform float heightoffset = 0.0;
uniform mat4 OSGModelViewProjectionMatrix;

void main(void) {
	vVertex = osg_Vertex;
	vNormal = osg_Normal;
    vTexCoord = osg_MultiTexCoord0;

    vec4 texData = texture2D(tex, osg_MultiTexCoord0);
    vColor = texData;
    float height = texData.a;
    if (channel == 0) height = texData.r;
    if (channel == 1) height = texData.g;
    if (channel == 2) height = texData.b;
    if (channel == 3) height = texData.a;
    vec4 tePosition = osg_Vertex;
    float nheight = height - heightoffset;
    if (local > 0) tePosition.xyz += osg_Normal * nheight * heightScale;
    else tePosition.y = nheight * heightScale;

#ifdef __EMSCRIPTEN__
    gl_Position = OSGModelViewProjectionMatrix * tePosition;
#else
    gl_Position = gl_ModelViewProjectionMatrix * tePosition;
#endif
}
);

string VRTerrain::fragmentShader_es2 =
GLSL(
#ifdef __EMSCRIPTEN__
precision mediump float;
#endif
uniform sampler2D texture;

varying vec2 vTexCoord;
varying vec4 vColor;

void main( void ) {
    //gl_FragColor = vec4(1.0,0.0,0.0,1.0);
    //gl_FragColor = vec4(vTexCoord.x,vTexCoord.y,0.0,1.0);
    gl_FragColor = texture2D(texture, vTexCoord);
}
);

string VRTerrain::fragmentShader =
"#version 400 compatibility\n"
GLSL(
uniform sampler2D tex;
uniform sampler2D texWoods;
uniform sampler2D texGravel;
uniform sampler2D texPic;
const ivec3 off = ivec3(-1,0,1);
const vec3 light = vec3(-1,-1,-0.5);
uniform vec2 texelSize;
uniform int doHeightTextures;
uniform vec4 mixColor;
uniform float mixAmount;
uniform float waterLevel;
uniform int isLit;

in vec4 pos;
in vec4 vertex;
in float height;
vec3 norm;
vec4 color;

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
	vec4  specular = 0.25*gl_LightSource[0].specular * pow( NdotHV, gl_FrontMaterial.shininess );
	gl_FragColor = ambient + diffuse + specular; //AGRAJAG
    //gl_FragColor = mix(diffuse + specular, vec4(0.7,0.9,1,1), clamp(1e-4*length(pos.xyz), 0.0, 1.0)); // atmospheric effects
	gl_FragColor[3] = 1.0;
	//gl_FragColor = vec4(1,0,0, 1);
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

void applyBumpMap(vec4 b) {
    float a = b.g*10;
    norm += 0.1*vec3(cos(a),0,sin(a));
    //norm.x += b.r*0.5;
    //norm.z += (b.g-1.0)*1.0;
    normalize(norm);
}

void main( void ) {
	vec2 tc = gl_TexCoord[0].xy;
	norm = getNormal();

	if (doHeightTextures == 0) color = vec4(getColor(),1.0);
	else {
        if (doHeightTextures == 1) {
            vec4 cW1 = texture(texWoods, tc*1077);
            vec4 cW2 = texture(texWoods, tc*107);
            vec4 cW3 = texture(texWoods, tc*17);
            vec4 cW4 = texture(texWoods, tc);
            vec4 cW = mix(cW1,mix(cW2,mix(cW3,cW4,0.5),0.5),0.5);
            //applyBumpMap(cW3);

            vec4 cG0 = texture(texGravel, tc*10777);
            vec4 cG1 = texture(texGravel, tc*1077);
            vec4 cG2 = texture(texGravel, tc*107);
            vec4 cG3 = texture(texGravel, tc*17);
            vec4 cG4 = texture(texGravel, tc);
            vec4 cG = mix(cG0,mix(cG1,mix(cG2,mix(cG3,cG4,0.5),0.5),0.5),0.5);
            if (height < waterLevel) color = vec4(0.2,0.4,1,1);
            else color = mix(cG, cW, min(cW3.r*0.1*max(height,0),1));
        } else {
            tc.y = 1-tc.y;
            color = texture(texPic, tc);
            color = mix(color, mixColor, mixAmount);
        }
	}

	if (isLit == 1) applyBlinnPhong();
	else gl_FragColor = color;//mix(color, vec4(1,1,1,1), 0.2);
}
);

string VRTerrain::fragmentShaderDeferred =
"#version 400 compatibility\n"
GLSL(
uniform sampler2D tex;
uniform sampler2D texWoods;
uniform sampler2D texGravel;
uniform sampler2D texPic;
const ivec3 off = ivec3(-1,0,1);
const vec3 light = vec3(-1,-1,-0.5);
uniform vec2 texelSize;
uniform vec2 texel;
uniform int doHeightTextures;
uniform vec4 mixColor;
uniform float mixAmount;
uniform float heightoffset = 0.0;
uniform float waterLevel;
uniform int isLit;
uniform vec3 atmoColor;
uniform float atmoThickness;

in vec4 vertex;
in vec4 pos;
in float height;
vec3 norm;
vec4 color;

vec3 getColor() {
	return texture2D(tex, gl_TexCoord[0].xy).rgb;
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
    vec3 n = normalize( cross(vb,va) );


    float k = texel.x;
    float _k = 1.0/k;
    if (tc.x > 1.0-k*1.5) n = mix(n, vec3(0,1,0), (tc.x-1.0+k*1.5)*_k);
    if (tc.y > 1.0-k*1.5) n = mix(n, vec3(0,1,0), (tc.y-1.0+k*1.5)*_k);
    if (tc.x < k*1.5) n = mix(n, vec3(0,1,0), 1.0-(tc.x-k*0.5)*_k);
    if (tc.y < k*1.5) n = mix(n, vec3(0,1,0), 1.0-(tc.y-k*0.5)*_k);
	return n;
}

void applyBumpMap(vec4 b) {
    float a = b.g*10;
    norm += 0.1*vec3(cos(a),0,sin(a));
    //norm.x += b.r*0.5;
    //norm.z += (b.g-1.0)*1.0;
    normalize(norm);
}

void main( void ) {
	vec2 tc = gl_TexCoord[0].xy;
	norm = getNormal();

	if (doHeightTextures == 0) color = vec4(getColor(),1.0);
	else {
        if ( doHeightTextures == 1 ) {
            vec4 cW1 = texture(texWoods, tc*1077);
            vec4 cW2 = texture(texWoods, tc*107);
            vec4 cW3 = texture(texWoods, tc*17);
            vec4 cW4 = texture(texWoods, tc);
            vec4 cW = mix(cW1,mix(cW2,mix(cW3,cW4,0.5),0.5),0.5);
            //applyBumpMap(cW3);

            vec4 cG0 = texture(texGravel, tc*10777);
            vec4 cG1 = texture(texGravel, tc*1077);
            vec4 cG2 = texture(texGravel, tc*107);
            vec4 cG3 = texture(texGravel, tc*17);
            vec4 cG4 = texture(texGravel, tc);
            vec4 cG = mix(cG0,mix(cG1,mix(cG2,mix(cG3,cG4,0.5),0.5),0.5),0.5);
            float nheight = height - heightoffset;
            if (nheight < waterLevel) color = vec4(0.2,0.4,1,1);
            else color = mix(cG, cW, min(cW3.r*0.1*max(nheight,0),1));
            color = mix(color, vec4(atmoColor,1), clamp(atmoThickness*length(pos.xyz), 0.0, 0.9)); // atmospheric effects
        } else {
            tc.y = 1-tc.y;
            color = texture(texPic, tc);
            color = mix(color, mixColor, mixAmount);
        }
	}

	//norm = normalize( gl_NormalMatrix * norm );
	norm = normalize( gl_NormalMatrix * norm ) + vec3(0,0,0.2); // bending normal towards camera to increase lightning
    gl_FragData[0] = vec4(vertex.xyz/vertex.w, 1.0);
    gl_FragData[1] = vec4(norm, isLit);
    gl_FragData[2] = color;
}
);

string VRTerrain::tessControlShader =
"#version 400 compatibility\n"
"#extension GL_ARB_tessellation_shader : enable\n"
GLSL(
layout(vertices = 4) out;
in vec3 vNormal[];
out vec3 tcPosition[];
out vec3 tcNormal[];
out vec2 tcTexCoords[];
uniform float resolution;
)
"\n#define ID gl_InvocationID\n"
GLSL(
void main() {
    tcPosition[ID] = gl_in[ID].gl_Position.xyz;
    tcNormal[ID] = vNormal[ID];
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
in vec3 tcNormal[];
in vec2 tcTexCoords[];
out float height;
out vec4 pos;
out vec4 vertex;

uniform float heightScale;
uniform int local = 0;
uniform int channel;
uniform float heightoffset = 0.0;
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
    vec3 c = mix(tcNormal[0], tcNormal[1], u);
    vec3 d = mix(tcNormal[3], tcNormal[2], u);
    vec3 teNormal = mix(c, d, v);
    height = heightScale * texture2D(texture, gl_TexCoord[0].xy)[channel];
    float nheight = (height - heightoffset);
    if (local > 0) tePosition += teNormal*nheight;
    else tePosition.y = nheight;
    pos = gl_ModelViewProjectionMatrix * vec4(tePosition, 1);
    vertex = gl_ModelViewMatrix * vec4(tePosition, 1);
    gl_Position = pos;
}
);

