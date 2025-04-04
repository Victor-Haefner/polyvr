#include "VRTerrain.h"
#include "VRPlanet.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRMaterialT.h"
#include "core/objects/material/VRTexture.h"
#include "core/objects/material/VRTextureGenerator.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/geometry/OSGGeometry.h"
#include "core/objects/geometry/VRMultiGrid.h"
#include "core/objects/VRCamera.h"
#include "core/utils/VRFunction.h"
#include "core/math/partitioning/boundingbox.h"
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
#include "core/utils/VRMutex.h"

#ifndef WITHOUT_BULLET
#include "core/objects/geometry/VRPhysics.h"
#include "VRTerrainPhysicsShape.h"
#endif



#define GLSL(shader) #shader

using namespace OSG;


VREmbankment::VREmbankment(PathPtr p1, PathPtr p2, PathPtr p3, PathPtr p4) : VRGeometry("embankment"), p1(p1), p2(p2), p3(p3), p4(p4) {
    for (auto p : p1->getPoints()) { auto pos = p->pos(); area.addPoint(Vec2d(pos[0],pos[2])); };
    for (auto p : p2->getPoints()) { auto pos = p->pos(); area.addPoint(Vec2d(pos[0],pos[2])); };
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


VRTerrainGrid::VRTerrainGrid() {}

void VRTerrainGrid::setRectangle(double width, double height) {
    p00 = Vec3d(-width*0.5, 0, -height*0.5);
    p10 = Vec3d( width*0.5, 0, -height*0.5);
    p01 = Vec3d(-width*0.5, 0,  height*0.5);
    p11 = Vec3d( width*0.5, 0,  height*0.5);
}

Vec2d VRTerrainGrid::approxSize() {
    double W = (p10[0] - p00[0] + p11[0] - p01[0])*0.5;
    double H = (p01[2] - p00[2] + p11[2] - p10[2])*0.5;
    return Vec2d(W, H);
}

Vec2d VRTerrainGrid::computeTexel(VRTexturePtr tex, int margin) {
    Vec3i s = tex->getSize();
    return Vec2d(1.0/(s[0]-margin), 1.0/(s[1]-margin));
}

Vec2d VRTerrainGrid::computeTexelSize(VRTexturePtr tex) {
    Vec2d gs = approxSize();
    Vec2d texel = computeTexel(tex, 1);
    Vec2d tSize;
    tSize[0] = gs[0]*texel[0];
    tSize[1] = gs[1]*texel[1];
    return tSize;
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
    grid.setRectangle(s[0], s[1]);

    resolution = r;
    heightScale = h;
#ifdef __EMSCRIPTEN__
    grid.grid = r;
#else
    grid.grid = r*64;
#endif
    mat->setShaderParameter("resolution", resolution);
    mat->setShaderParameter("heightScale", heightScale);
    mat->setShaderParameter("doHeightTextures", 0);
    mat->setShaderParameter("waterLevel", w);
    mat->setShaderParameter("isLit", int(isLit));
    mat->setShaderParameter("invertSatY", int(doInvertSatY));
    mat->setShaderParameter("invertTopoY", int(doInvertTopoY));
    mat->setShaderParameter("atmoColor", aC);
    mat->setShaderParameter("atmoThickness", aT);
    updateTexelSize();
    //setupGeo();
}

void VRTerrain::setLocalized(bool in){ localMesh = in; }
void VRTerrain::setLODFactor(double in){ LODfac = in; }
double VRTerrain::getLODFactor(){ return LODfac; }
void VRTerrain::setMeshTer(vector<vector<vector<Vec3d>>> in){ meshTer = in; }
void VRTerrain::setWaterLevel(float w) { mat->setShaderParameter("waterLevel", w); }
void VRTerrain::setLit(bool isLit) { mat->setShaderParameter("isLit", int(isLit)); }
void VRTerrain::setAtmosphericEffect(float thickness, Color3f color) { mat->setShaderParameter("atmoColor", color); mat->setShaderParameter("atmoThickness", thickness); }
void VRTerrain::setHeightScale(float s) { heightScale = s; mat->setShaderParameter("heightScale", s); }

void VRTerrain::setInvertY(bool invertSatY, bool invertTopoY) {
    doInvertSatY = invertSatY;
    doInvertTopoY = invertTopoY;
    mat->setShaderParameter("invertSatY", int(doInvertSatY));
    mat->setShaderParameter("invertTopoY", int(doInvertTopoY));
}

void VRTerrain::setMap( VRTexturePtr t, int channel, Vec4d rect ) {
    VRLock lock(mtx());
    if (!t) return;
    heigthsTex = t;
    heightsRect = rect;
    mat->setTexture(heigthsTex);
    mat->clearTransparency();
    mat->setShaderParameter("channel", channel);
    mat->setShaderParameter("heightoffset", heightoffset);
    mat->setTextureParams(GL_LINEAR, GL_LINEAR, GL_MODULATE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
    mat->clearTransparency();
    updateTexelSize();
    setupGeo();
}

VRTexturePtr VRTerrain::getTexture() { return satTex; }

void VRTerrain::setTexture(VRTexturePtr t, Color4f mCol, float mAmount, Vec4d rect) {
    satTex = t;
    satRect = rect;
    mat->setTexture(satTex, 0, 3);
    mat->setTextureWrapping(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, 3);
    mat->setShaderParameter("mixColor", mCol);
    mat->setShaderParameter("mixAmount", mAmount);
    mat->setShaderParameter("texPic", 3);
    mat->setShaderParameter("doHeightTextures", 2);
    mat->clearTransparency();
}

void VRTerrain::paintHeights(string woods, string gravel) {
    mat->setTexture(woods, 0, 1);
    mat->setTexture(gravel, 0, 2);
    mat->setShaderParameter("texWoods", 1);
    mat->setShaderParameter("texGravel", 2);
    mat->setShaderParameter("doHeightTextures", 1);
    mat->clearTransparency();
}

void VRTerrain::paintHeights(string path, Color4f mCol, float mAmount, Vec4d rect) {
    auto t = VRTexture::create();
    t->read(path);
    setTexture(t, mCol, mAmount, rect);
}

void VRTerrain::paintHeights(VRTexturePtr tex, Color4f mCol, float mAmount, Vec4d rect) {
    setTexture(tex, mCol, mAmount, rect);
}

void VRTerrain::updateTexelSize() {
    if (!heigthsTex) return;
    texelSize = grid.computeTexelSize(heigthsTex);
    Vec2d texel = grid.computeTexel(heigthsTex, 1);
	mat->setShaderParameter("texel", Vec2f(texel));
	mat->setShaderParameter("texelSize", Vec2f(texelSize));
}

void VRTerrain::curveMesh(VRPlanetPtr p, Vec2d c, PosePtr s) {
    localMesh = true;
    planet = p;
    planetCoords = c;
    pSectorInv = s;
}

Vec2d computeGridSpacing(Vec2d gSize, Vec2d gridSize, double res) {
    Vec2i gridN = Vec2i(round(gSize[0]*1.0/res-0.5), round(gSize[1]*1.0/res-0.5));
    if (gridN[0] < 1) gridN[0] = 1;
    if (gridN[1] < 1) gridN[1] = 1;
    Vec2d gridS = gridSize;
    gridS[0] /= gridN[0];
    gridS[1] /= gridN[1];
    return gridS;
}

bool VRTerrain::createMultiGrid(VRCameraPtr cam, double res) {
    auto modulo = [](const double& a, const double& b) { return a-floor(a/b)*b; };
    auto checkChange = [&](const vector<double>& v, double eps = 0) {
        if (v.size() != oldMgParams.size()) { oldMgParams = v; return true; }
        for (int i=0; i<v.size(); i++)
            if (abs(oldMgParams[i]-v[i]) > eps) { oldMgParams = v; return true; }
        return false;
    };

    Vec2d gSize = grid.approxSize();
    Vec2d sectorSize = gSize;
    double N1 = -sectorSize[0]*0.5;
    double N2 =  sectorSize[0]*0.5;
    double E1 = -sectorSize[1]*0.5;
    double E2 =  sectorSize[1]*0.5;

    auto pla = planet.lock();
    if (pla) {
        sectorSize[0] = pla->getSectorSize();
        sectorSize[1] = pla->getSectorSize();
        N1 = planetCoords[0];
        N2 = planetCoords[0]+sectorSize[0];
        E1 = planetCoords[1];
        E2 = planetCoords[1]+sectorSize[1];
    }

#ifdef __EMSCRIPTEN__
    Vec2d NE;
    PosePtr camPose;
    if (pla) {
	    if (cam) {
		camPose = cam->getWorldPose();
		NE = pla->fromPosLatLong(camPose->pos(), 1, 0);
		if (NE[0] < N1 || NE[0] > N2 || NE[1] < E1 || NE[1] > E2) { // cam not above terrain
		    res *= 1e6;
		    if (!checkChange(vector<double>())) return false;
		}
		if (!checkChange(vector<double>({NE[0], NE[1]}))) return false;
	    } else {
		res *= 1e6;
		if (!checkChange(vector<double>())) return false;
	    }
    }

    if (pla) res *= 32;
    else res *= 64;
    Vec2d r1 = computeGridSpacing(gSize, sectorSize, res);
    if (!checkChange(vector<double>({res, r1[0], r1[1], NE[0], NE[1]}))) return false;
#else
    Vec2d r1 = computeGridSpacing(gSize, sectorSize, res);
#endif

    /*if (camPose) cout << " ---- camPose " << camPose->pos() << endl;
    cout << " ---- NE " << NE << endl;
    cout << " ---- grid " << grid.grid << endl;
    cout << " ---- resolution " << resolution << endl;
    cout << " ---- res " << res << endl;
    cout << " ---- r1 " << r1 << endl;*/
    VRMultiGrid mg("terrainGrid");
    mg.addGrid(Vec4d(N1,N2,E1,E2), Vec2d(r1[0],r1[1]));

#ifdef __EMSCRIPTEN__
    if (pla) {
	    Vec2d r2 = r1/4.0;
	    Vec2d r3 = r2/8.0;

	    Vec2d L1 = r1*2*0.9;
	    Vec2d L2 = r2*2*0.9;

	    double x = (N1+N2)*0.5;
	    double y = (E1+E2)*0.5;
	    if (cam) {
		x = NE[0] - camPose->dir()[2]*L2[0];
		y = NE[1] + camPose->dir()[0]*L2[1];
	    }

	    x -= modulo(x,r1[0]); // x%r1
	    y -= modulo(y,r1[1]); // y%r1
	    Vec4d rect1 = Vec4d(x-L1[0], x+L1[0], y-L1[1], y+L1[1]);
	    Vec4d rect2 = Vec4d(x-L2[0], x+L2[0], y-L2[1], y+L2[1]);

	    //cout << " - createMultiGrid r1: " << r1 << " r2: " << r2 << " r3: " << r3 << " xy " << Vec2d(x,y) << " L1 " << L1 << " L2 " << L2 << endl;

	    if (r2[0] < r1[0] && 2*L1[0] < (N2-N1) && x-L1[0] > N1 && x+L1[0] < N2) mg.addGrid(rect1, Vec2d(r2[0],r2[1]));
	    if (r3[0] < r2[0] && 2*L2[0] < 2*L1[0] && x-L2[0] > N1 && x+L2[0] < N2) mg.addGrid(rect2, Vec2d(r3[0],r3[1]));
    }
#endif
    if (!mg.compute(ptr())) {
        cout << " Error in VRTerrain::createMultiGrid, compute failed! ..abort" << endl;
        return false;
    }

    if (!heigthsTex) return false;
    Vec2d texel = grid.computeTexel(heigthsTex, 1);

    VRGeoData data(ptr());
    if (!getMesh() || !getMesh()->geo) return false;

    auto positions = getMesh()->geo->getPositions();
    for (size_t i=0; i<positions->size(); i++) {
        Pnt3f pos = positions->getValue<Pnt3f>(i);
        double N = pos[0];
        double E = pos[2];
        double tn = (N-N1)/(N2-N1);
        double te = (E-E1)/(E2-E1);

        if (pla) {
            double tcx = texel[0]*0.5 + te*((heightsRect[2]-heightsRect[0])-texel[0]);
            double tcy = texel[1]*0.5 + tn*((heightsRect[3]-heightsRect[1])-texel[1]);
            Vec2d tc0 = Vec2d(heightsRect[0]+tcx, 1.0 - (heightsRect[1]+tcy)); // depth
            Vec2d tc1 = Vec2d(satRect[0]+te*(satRect[2]-satRect[0]), 1.0 - (satRect[1]+tn*(satRect[3]-satRect[1]))); // sat
            data.pushTexCoord(tc0, 0);
            data.pushTexCoord(tc1, 1);
            auto pose = pla->fromLatLongPose(N, E);
            pose = pSectorInv->multRight(pose);
            data.setPos(i, pose->pos());
            data.setNorm(i, pose->up());
        } else {
            double tcx = texel[0]*0.5 + tn*((heightsRect[2]-heightsRect[0])-texel[0]);
            double tcy = texel[1]*0.5 + te*((heightsRect[3]-heightsRect[1])-texel[1]);
            data.pushTexCoord(Vec2d(heightsRect[0]+tcx, (heightsRect[1]+tcy)), 0); // depth
            data.pushTexCoord(Vec2d(satRect[0]+tn*(satRect[2]-satRect[0]), (satRect[1]+te*(satRect[3]-satRect[1]))), 1);   // sat
        }
    }

    data.apply(ptr());
    //cout << " -- createMultiGrid " << positions->size() << endl;
    return true;
}

void VRTerrain::setupGeo(VRCameraPtr cam) {
    //cout << "VRTerrain::setupGeo" << endl;
    if (!createMultiGrid(cam, grid.grid)) return;

#ifndef __EMSCRIPTEN__
    setType(GL_PATCHES);
    setPatchVertices(4);
#endif

    if (localMesh && planet.lock()) mat->setShaderParameter("local",1);
    setMaterial(mat);
    //cout << "VRTerrain::setupGeo done" << endl;
}

vector<Vec3d> VRTerrain::probeHeight( Vec2d p ) {
    if (!heigthsTex) return {};
    Vec2d uv = toUVSpace(p);
    Vec2d uvP = fromUVSpace(uv);
    int i = round(uv[0]-0.5);
    int j = round(uv[1]-0.5);

    double h00 = heigthsTex->getPixelVec(Vec3i(i,j,0))[0];
    double h10 = heigthsTex->getPixelVec(Vec3i(i+1,j,0))[0];
    double h01 = heigthsTex->getPixelVec(Vec3i(i,j+1,0))[0];
    double h11 = heigthsTex->getPixelVec(Vec3i(i+1,j+1,0))[0];

    double u = uv[0]-i;
    double v = uv[1]-j;
    double h = ( h00*(1-u) + h10*u )*(1-v) + ( h01*(1-u) + h11*u )*v;

    Vec2d p0 = fromUVSpace( Vec2d(i,j) );
    Vec2d p1 = fromUVSpace( Vec2d(i+1,j+1) );
    //cout << " probeHeight uv:" << uv << " i:" << i << " j:" << j << " size:" << grid.size << " p0:" << p0 << " p1:" << p1 << endl;
    for (auto e : embankments) if (e.second->isInside(p)) return e.second->probeHeight(p);

    return {Vec3d(p[0], h, p[1]),
            Vec3d(p0[0], h00, p0[1]),
            Vec3d(p1[0], h10, p0[1]),
            Vec3d(p0[0], h01, p1[1]),
            Vec3d(p1[0], h11, p1[1]) };
}

VRTexturePtr VRTerrain::getMap() { return heigthsTex; }
Vec2d VRTerrain::getTexelSize() { return texelSize; }

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

    vrPhysicalize();
    getPhysics()->setFriction(0.8);
    getPhysics()->setPhysicalized(true);
#endif
}

Boundingbox VRTerrain::getBoundingBox() {
    Boundingbox bb;
    bb.update( grid.p00 );
    bb.update( grid.p10 );
    bb.update( grid.p01 );
    bb.update( grid.p11 );
    if (!heigthsTex) return bb;

    float hmax = -1e30;
    float hmin = 1e30;
    for (int i=0; i<heigthsTex->getSize()[0]; i++) {
        for (int j=0; j<heigthsTex->getSize()[1]; j++) {
            auto h = heigthsTex->getPixelVec(Vec3i(i,j,0))[0];
            if (h < hmin) hmin = h;
            if (h > hmax) hmax = h;
        }
    }

    bb.update( Vec3d(0, hmax, 0) );
    bb.update( Vec3d(0, hmin, 0) );
    return bb;
}

void VRTerrain::setSimpleNoise() {
    VRLock lock(mtx());
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

VRMutex& VRTerrain::mtx() {
#ifndef WITHOUT_BULLET
    auto scene = VRScene::getCurrent();
    if (scene) return scene->physicsMutex();
    else
#endif
    {
        static VRMutex m;
        return m;
    };
}

void VRTerrain::setHeightTexture(VRTexturePtr t) {
    VRLock lock(mtx());
    heigthsTex = t;
}

void VRTerrain::setupMat() {
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
    mat->setShaderParameter("texture", 0);
    mat->setShaderParameter("channel", 3);
    mat->setZOffset(1,1);
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

/**

TODO: implement this in getTexCoord


The approach of the bilinear interpolation
p1 := a + ( b - a ) * u
p2 := d + ( c - d ) * u
p := p1 + ( p2 - p1 ) * v

means that there is a line between p1 and p2 that passes through p. Expressing this line the other way, namely
p + l = p1
p + k * l = p2
so that l + k * l (where k is obviously need to be a negative number) is the said line.
Considering that this is done in 3D, we have 5 equations with 5 unkowns (k, lx, ly, lz, and u)
where k * lx, k * ly and k * lz are the problems.

Fortunately, from the lower of the both equations, we can isolate
k * lx = dx - px + ( cx - dx ) * u
k * ly = dy - py + ( cy - dy ) * u
k * lz = dz - pz + ( cz - dz ) * u

and divide both of these, so that
lx / ly = [ dx - px + ( cx - dx ) * u ] / [ dy - py + ( cy - dy ) * u ]
lx / lz = [ dx - px + ( cx - dx ) * u ] / [ dz - pz + ( cz - dz ) * u ]

Now, using the upper of the equations to get
lx = ax - px + ( bx - ax ) * u
ly = ay - py + ( by - ay ) * u
lz = az - pz + ( bz - az ) * u
and setting these into the above lx / ly, we get quadratic equations solely in u

[ dx - px + ( cx - dx ) * u ] / [ dy - py + ( cy - dy ) * u ] = [ax - px + ( bx - ax ) * u] / [ay - py + ( by - ay ) * u]

=>
[ dx - px + ( cx - dx ) * u ] * [ay - py + ( by - ay ) * u] = [ax - px + ( bx - ax ) * u] * [ dy - py + ( cy - dy ) * u ]

=>

...... TODO: solve for u then k

*/

Vec2d VRTerrain::getTexCoord( Vec2d p ) {
    if (!heigthsTex) return Vec2d();
    Vec2d texel = grid.computeTexel(heigthsTex, 1);

    // normalized coords
    Vec2d gSize = grid.approxSize();
    double U = p[0]/gSize[0];
    double V = p[1]/gSize[1];

    double u = (1.0-texel[0])*U + 0.5;
    double v = (1.0-texel[1])*V + 0.5;
    if (doInvertTopoY) return Vec2d(u,1.0-v);
    else return Vec2d(u,v);
}

Vec2d VRTerrain::toUVSpace(Vec2d p) {
    if (!heigthsTex) return Vec2d();
    Vec3i texSize = heigthsTex->getSize();
    Vec2d uv = getTexCoord(p);
    int W = texSize[0]-1;
    int H = texSize[1]-1;
    return Vec2d(uv[0]*W, uv[1]*H);
}

Vec2d VRTerrain::fromUVSpace(Vec2d uv) {
    if (!heigthsTex) return Vec2d();

    Vec2d texel = grid.computeTexel(heigthsTex, 1);
    Vec3i texSize = heigthsTex->getSize();
    double W = texSize[0]-1;
    double H = texSize[1]-1;
    uv[0] /= W;
    uv[1] /= H;
    if (doInvertTopoY) uv[1] = 1.0-uv[1];

    Vec2d gSize = grid.approxSize();
    double x = (uv[0]-0.5)*gSize[0]/(1.0-texel[0]);
    double z = (uv[1]-0.5)*gSize[1]/(1.0-texel[1]);
    return Vec2d(x,z);
}

double VRTerrain::getHeight(Vec2d p, bool useEmbankments) {
    if (!heigthsTex) return 0;
    Vec2d uv = toUVSpace(p); // uv, i and j are tested
    int i = round(uv[0]-0.5);
    int j = round(uv[1]-0.5);

    double h00 = heigthsTex->getPixelVec(Vec3i(i,j,0))[0];
    double h10 = heigthsTex->getPixelVec(Vec3i(i+1,j,0))[0];
    double h01 = heigthsTex->getPixelVec(Vec3i(i,j+1,0))[0];
    double h11 = heigthsTex->getPixelVec(Vec3i(i+1,j+1,0))[0];

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
    VRLock lock(mtx());
    VRPolygonPtr poly = VRPolygon::create();
    for (auto p : perimeter) poly->addPoint(p);
    Vec2d gSize = grid.approxSize();
    poly->scale( Vec3d(1.0/gSize[0], 1, 1.0/gSize[1]) );
    poly->translate( Vec3d(0.5,0,0.5) );

    auto dim = heigthsTex->getSize();
    for (int i = 0; i < dim[0]; i++) {
        for (int j = 0; j < dim[1]; j++) {
            auto pix = Vec2d(i*1.0/(dim[0]-1), j*1.0/(dim[1]-1));
            if (poly->isInside(pix)) {
                Vec3i pixK = Vec3i(i,j,0);
                Color4f col = heigthsTex->getPixelVec(pixK);
                col[0] = h;
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
            double h = tex->getPixel(pixK)[0];
            auto pix = Vec2d(i*1.0/(dim[0]-1), j*1.0/(dim[1]-1));
            //if (tgPolygon->isInside(pix)) h = 14;
            Color4f col = t->getPixel(pixK);
            col[0] = h;
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

Vec2d VRTerrain::getSize() { return grid.approxSize(); }
double VRTerrain::getGrid() { return grid.grid; }

// --------------------------------- shader ------------------------------------

string VRTerrain::vertexShader =
"#version 120\n"
GLSL(
attribute vec4 osg_Vertex;
attribute vec3 osg_Normal;
attribute vec2 osg_MultiTexCoord0;
attribute vec2 osg_MultiTexCoord1;
varying vec3 vNormal;

void main(void) {
    gl_TexCoord[0] = vec4(osg_MultiTexCoord0,0.0,0.0);
    gl_TexCoord[1] = vec4(osg_MultiTexCoord1,0.0,0.0);
	gl_Position = osg_Vertex;
	vNormal = osg_Normal;
}
);

string VRTerrain::vertexShader_es2 =
#ifdef __EMSCRIPTEN__
"#version 300 es\n"
#endif
GLSL(

#ifdef __EMSCRIPTEN__
in vec4 osg_Vertex;
in vec3 osg_Normal;
in vec2 osg_MultiTexCoord0;
in vec2 osg_MultiTexCoord1;
out vec3 vNormal;
out vec4 vColor;
out vec4 vVertex;
out vec2 vTexCoord;
out vec2 vTexCoord2;
#else
attribute vec4 osg_Vertex;
attribute vec3 osg_Normal;
attribute vec2 osg_MultiTexCoord0;
attribute vec2 osg_MultiTexCoord1;
varying vec3 vNormal;
varying vec4 vColor;
varying vec4 vVertex;
varying vec2 vTexCoord;
varying vec2 vTexCoord2;
#endif

uniform sampler2D tex;
uniform float heightScale;
uniform int local;
uniform int channel;
uniform int invertTopoY;
uniform float heightoffset;
uniform mat4 OSGModelViewProjectionMatrix;

void main(void) {
    vVertex = osg_Vertex;
    vNormal = osg_Normal;
    vTexCoord = osg_MultiTexCoord0;
    vTexCoord2 = osg_MultiTexCoord1;

    vec2 tc = osg_MultiTexCoord0
    if (invertTopoY == 1) tc.y = 1-tc.y;
    vec4 texData = texture(tex, tc);
    vColor = texData;
    float height = texData.r;
    vec4 tePosition = osg_Vertex;
    //float nheight = osg_MultiTexCoord0.x * 10000.0;
    float nheight = (height - heightoffset);//*100.0;
    if (local > 0) tePosition.xyz += osg_Normal * nheight * heightScale;
    else tePosition.y = nheight * heightScale;
    //tePosition.y = nheight * heightScale;

#ifdef __EMSCRIPTEN__
    gl_Position = OSGModelViewProjectionMatrix * tePosition;
#else
    gl_Position = gl_ModelViewProjectionMatrix * tePosition;
#endif
}
);

string VRTerrain::fragmentShader_es2 =
#ifdef __EMSCRIPTEN__
"#version 300 es\n"
#endif
GLSL(
#ifdef __EMSCRIPTEN__
precision mediump float;
#endif
uniform sampler2D tex;
uniform sampler2D texPic;
uniform mat4 OSGNormalMatrix;
uniform vec2 texelSize;
uniform int isLit;
uniform int invertSatY;
uniform int invertTopoY;

#ifdef __EMSCRIPTEN__
in vec2 vTexCoord;
in vec2 vTexCoord2;
in vec4 vColor;
out vec4 fragColor;
#else
varying vec2 vTexCoord;
varying vec2 vTexCoord2;
varying vec4 vColor;
#endif

vec3 norm;
vec4 color;
const ivec3 off = ivec3(-1,0,1);

void applyBlinnPhong() {
/*#ifndef __EMSCRIPTEN__
	norm = normalize( gl_NormalMatrix * norm );
#else
	norm = normalize( (OSGNormalMatrix * vec4(norm,0.0)).xyz );
#endif
	vec3  light = normalize( (OSGNormalMatrix * vec4(1.0,5.0,2.0,0.0)).xyz ); // directional light*/
        vec3  light = normalize( vec3(1.0,5.0,2.0) ); // directional light in mesh coords
	float NdotL = max(dot( norm, light ), 0.0);
	vec4  ambient = vec4(0.5,0.5,0.5,1.0) * color;
	vec4  diffuse = vec4(1.0,1.0,0.9,1.0) * NdotL * color;
	fragColor = ambient + diffuse;
	fragColor[3] = 1.0;
}

vec3 getNormal() {
    vec2 tc = vTexCoord;
    if (invertTopoY == 1) tc.y = 1-tc.y;
    float s11 = texture(tex, tc).r;
    float s01 = textureOffset(tex, tc, off.xy).r;
    float s21 = textureOffset(tex, tc, off.zy).r;
    float s10 = textureOffset(tex, tc, off.yx).r;
    float s12 = textureOffset(tex, tc, off.yz).r;

    vec2 r2 = 2.0*texelSize;
    vec3 va = normalize(vec3(r2.x,s21-s01,0));
    vec3 vb = normalize(vec3(   0,s12-s10,r2.y));
    vec3 n = cross(vb,va);
    return n;
}

void main( void ) {
    vec2 tc = vTexCoord2;
    if (invertSatY == 1) tc.y = 1.0 - tc.y;
    color = texture(texPic, tc);

    if (isLit == 1) {
        norm = getNormal();
        applyBlinnPhong();
    } else fragColor = color;
    //fragColor = vec4(1.0,0.0,0.0,1.0);
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
uniform int invertSatY;
uniform int invertTopoY;

in vec4 pos;
in vec4 vertex;
in float height;
vec3 norm;
vec4 color;

void applyBlinnPhong() {
	norm = normalize( gl_NormalMatrix * norm );
	vec3  light = normalize( gl_LightSource[0].position.xyz ); // directional light
	float NdotL = max(dot( norm, light ), 0.0);
	vec4  ambient = gl_LightSource[0].ambient * color;
	vec4  diffuse = gl_LightSource[0].diffuse * NdotL * color;
	float NdotHV = max(dot( norm, normalize(gl_LightSource[0].halfVector.xyz)),0.0);
	vec4  specular = 0.25*gl_LightSource[0].specular * pow( NdotHV, gl_FrontMaterial.shininess );
	gl_FragColor = ambient + diffuse + specular;
	gl_FragColor[3] = 1.0;
}

vec3 getNormal() {
    vec2 tc = gl_TexCoord[0].xy;
    if (invertTopoY == 1) tc.y = 1-tc.y;
    float s11 = texture(tex, tc).r;
    float s01 = textureOffset(tex, tc, off.xy).r;
    float s21 = textureOffset(tex, tc, off.zy).r;
    float s10 = textureOffset(tex, tc, off.yx).r;
    float s12 = textureOffset(tex, tc, off.yz).r;

    vec2 r2 = 2.0*texelSize;
    vec3 va = normalize(vec3(r2.x,s21-s01,0));
    vec3 vb = normalize(vec3(   0,s12-s10,r2.y));
    vec3 n = cross(vb,va);
    return n;
}

void main( void ) {
	vec2 tc = gl_TexCoord[1].xy;
	norm = getNormal();

	if (doHeightTextures == 0) color = vec4(texture2D(tex, tc).rgb, 1.0);
	else {
        if (doHeightTextures == 1) {
            if (invertSatY == 1) tc.y = 1-tc.y;
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
            if (height < waterLevel) color = vec4(0.2,0.4,1,1);
            else color = mix(cG, cW, min(cW3.r*0.1*max(height,0),1));
        } else {
            if (invertSatY == 1) tc.y = 1-tc.y;
            color = texture(texPic, tc);
            color = mix(color, mixColor, mixAmount);
        }
	}

	if (isLit == 1) applyBlinnPhong();
	else gl_FragColor = color;
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
uniform int invertSatY;
uniform int invertTopoY;
uniform vec3 atmoColor;
uniform float atmoThickness;

in vec4 vertex;
in vec4 pos;
in float height;
vec3 norm;
vec4 color;

vec3 getNormal() {
	vec2 tc = gl_TexCoord[0].xy;
    if (invertTopoY == 1) tc.y = 1-tc.y;
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
	vec2 tc = gl_TexCoord[1].xy;
	norm = getNormal();

	if (doHeightTextures == 0) color = vec4( texture2D(tex, tc).rgb ,1.0);
	else {
        if ( doHeightTextures == 1 ) {
            if (invertSatY == 1) tc.y = 1-tc.y;
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
            if (invertSatY == 1) tc.y = 1-tc.y;
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
out vec2 tcTexCoords2[];
uniform float resolution;
)
"\n#define ID gl_InvocationID\n"
GLSL(
void main() {
    tcPosition[ID] = gl_in[ID].gl_Position.xyz;
    tcNormal[ID] = vNormal[ID];
    tcTexCoords[ID] = gl_in[ID].gl_TexCoord[0].xy;
    tcTexCoords2[ID] = gl_in[ID].gl_TexCoord[1].xy;

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
in vec2 tcTexCoords2[];
out float height;
out vec4 pos;
out vec4 vertex;

uniform float heightScale;
uniform int local = 0;
uniform int channel;
uniform int invertTopoY;
uniform float heightoffset = 0.0;
uniform sampler2D texture;

void main() {
    float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;

    vec2 ta = mix(tcTexCoords[0], tcTexCoords[1], u);
    vec2 tb = mix(tcTexCoords[3], tcTexCoords[2], u);
    vec2 tc = mix(ta, tb, v);
    gl_TexCoord[0] = vec4(tc.x, tc.y, 1.0, 1.0);

    vec2 ta2 = mix(tcTexCoords2[0], tcTexCoords2[1], u);
    vec2 tb2 = mix(tcTexCoords2[3], tcTexCoords2[2], u);
    vec2 tc2 = mix(ta2, tb2, v);
    gl_TexCoord[1] = vec4(tc2.x, tc2.y, 1.0, 1.0);

    vec3 a = mix(tcPosition[0], tcPosition[1], u);
    vec3 b = mix(tcPosition[3], tcPosition[2], u);
    vec3 tePosition = mix(a, b, v);
    vec3 c = mix(tcNormal[0], tcNormal[1], u);
    vec3 d = mix(tcNormal[3], tcNormal[2], u);
    vec3 teNormal = mix(c, d, v);
    vec2 ttc = gl_TexCoord[0].xy;
    if (invertTopoY == 1) ttc.y = 1-ttc.y;
    vec4 texData = texture2D(texture, ttc);
    height = heightScale * texData[channel];
    float nheight = (height - heightoffset);
    if (local > 0) tePosition += teNormal*nheight;
    else tePosition.y = nheight;
    pos = gl_ModelViewProjectionMatrix * vec4(tePosition, 1);
    vertex = gl_ModelViewMatrix * vec4(tePosition, 1);
    gl_Position = pos;
}
);

