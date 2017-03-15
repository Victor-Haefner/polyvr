#include "ModuleStreets.h"

#include "core/objects/material/VRMaterial.h"
#include "core/scene/VRSceneManager.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/tools/VRAnnotationEngine.h"
#include "core/objects/geometry/VRPhysics.h"
#include "core/utils/toString.h"
#include "core/math/pose.h"

#include "../Config.h"
#include "../RealWorld.h"
#include "../OSM/OSMMap.h"
#include "../OSM/OSMMapDB.h"
#include "../MapCoordinator.h"
#include "../StreetAlgos.h"
#include "../Assets/StreetLamp.h"
#include "StreetJoint.h"
#include "StreetSegment.h"
#include <boost/exception/to_string.hpp>
#include <OpenSG/OSGMatrixUtility.h>

using namespace OSG;

ModuleStreets::ModuleStreets(bool t, bool p) : BaseModule("ModuleStreets", t,p) {
    //this->streetHeight = Config::STREET_HEIGHT + Config::GROUND_LVL;

    // create material
    matStreet = VRMaterial::create("Street");
    matStreet->setTexture("world/textures/street2.png");

    matStreet->setAmbient(Color3f(0.5, 0.5, 0.5)); //light reflection in all directions
    matStreet->setDiffuse(Color3f(1.0, 1.0, 1.0)); //light from ambient (without lightsource)
    matStreet->setSpecular(Color3f(0.2, 0.2, 0.2)); //light reflection in camera direction
    string wdir = VRSceneManager::get()->getOriginalWorkdir();
    matStreet->readVertexShader(wdir+"/shader/TexturePhong/phong.vp");
    matStreet->readFragmentShader(wdir+"/shader/TexturePhong/phong.fp");
    matStreet->setMagMinFilter(GL_NEAREST, GL_NEAREST, 0);
    matStreet->setZOffset(-1,-1);

    matSigns = VRMaterial::create("Signs");
    matSigns->setTexture("world/streetsigns2.png");
    matSigns->setLit(0);

    matLights = VRMaterial::create("Lights");
    matLights->setTexture("world/lights.png");
    matLights->readVertexShader(wdir+"/shader/TexturePhong/phong.vp");
    matLights->readFragmentShader(wdir+"/shader/TexturePhong/phong.fp");

    // Autobahn
    float W = Config::get()->STREET_WIDTH;
    float BH = Config::get()->Config::get()->BRIDGE_HEIGHT;
    types["motorway"] = StreetType("motorway", W, BH, matStreet, true);
    types["motorway_link"] = StreetType("motorway_link", W, BH, matStreet, true);

    // landstrasse
    types["trunk"] = StreetType("trunk", W, BH, matStreet, true);
    types["trunk_link"] = StreetType("trunk_link", W, BH, matStreet, true);
    types["primary"] = StreetType("primary", W, BH, matStreet, true);
    types["primary_link"] = StreetType("primary_link", W, BH, matStreet, true);
    types["secondary"] = StreetType("secondary", W*0.9, BH, matStreet, true);
    types["secondary_link"] = StreetType("secondary_link", W*0.9, BH, matStreet, true);
    types["tertiary"] = StreetType("tertiary", W*0.8, BH, matStreet, true);
    types["tertiary_link"] = StreetType("tertiary_link", W*0.8, BH, matStreet, true);
    types["unclassified"] = StreetType("unclassified", W*0.7, BH, matStreet, true);

    // stadtstrassen
    types["residential"] = StreetType("residential", W*0.8, BH*0.6, matStreet, true);

    // fusgaenger
    types["living_street"] = StreetType("living_street", W*0.4, BH*0.6, matStreet, false);
    types["service"] = StreetType("service", W*0.4, BH*0.6, matStreet, false);
    types["raceway"] = StreetType("raceway", W*0.4, BH*0.6, matStreet, false);
    types["cycleway"] = StreetType("cycleway", W*0.4, BH*0.6, matStreet, false);
    types["path"] = StreetType("path", W*0.4, BH*0.6, matStreet, false);
    types["pedestrian"] = StreetType("pedestrian", W*0.4, BH*0.6, matStreet, false);
    types["bridleway"] = StreetType("bridleway", W*0.4, BH*0.6, matStreet, false);
    types["track"] = StreetType("track", W*0.4, BH*0.6, matStreet, false);
    types["footway"] = StreetType("footway", W*0.4, BH*0.6, matStreet, false);

    types["road"] = StreetType("road", W, BH, matStreet, false); // tmp tag

    // sign texture coordinates
    signTCs["noEntry"] = Vec4f(0.6242,0.6242+0.0385,1.0-0.5766-0.0559,1.0-0.5766);
    signTCs["straightOrRight"] = Vec4f(0.7324,0.7324+0.0348,1.0-0.5766-0.0519,1.0-0.5766);
}

void ModuleStreets::loadBbox(MapGrid::Box bbox) {
    auto mapDB = RealWorld::get()->getDB();
    auto mc = RealWorld::get()->getCoordinator();
    if (!mapDB || !mc) return;
    OSMMap* osmMap = mapDB->getMap(bbox.str);
    if (!osmMap) return;

    map<string, StreetJoint*> listLoadJoints;
    map<string, StreetSegment*> listLoadSegments;

    for (OSMNode* node : osmMap->osmNodes) { // Load StreetJoints
        Vec2f pos = mc->realToWorld(Vec2f(node->lat, node->lon));
        StreetJoint* joint = new StreetJoint(pos, node->id);
        listLoadJoints[node->id] = joint;
    }

    for (OSMWay* way : osmMap->osmWays) {
        if (!types.count(way->tags["highway"])) continue;
        auto type = types[way->tags["highway"]];

        for (unsigned int i=0; i < way->nodeRefs.size()-1; i++) {
            string nodeId1 = way->nodeRefs[i];
            string nodeId2 = way->nodeRefs[i+1];
            string segId = way->id + "-" + boost::to_string(i);

            StreetSegment* seg = new StreetSegment(listLoadJoints[nodeId1], listLoadJoints[nodeId2], type.width, segId);
            seg->bridge = ( way->tags["tunnel"] == "yes" || way->tags["bridge"] == "yes" ); // workaround: treat tunnels as bridges
            seg->bridgeHeight = type.bridgeHeight;
            listLoadSegments[segId] = seg;

            if (way->tags.count("lanes")) seg->lanes = toInt(way->tags["lanes"].c_str());
            //if (type.type == "secondary") seg->lanes = 2;
            seg->lanes = max(1, seg->lanes);
            seg->width = seg->width * seg->lanes;

            if (way->tags.count("name")) seg->name = way->tags["name"];

            listLoadJoints[nodeId1]->segments.push_back(seg);
            listLoadJoints[nodeId2]->segments.push_back(seg);
        }
    }

    // prepare joints
    for (auto jointId : listLoadJoints) {
        StreetJoint* joint = jointId.second;
        if (joint->segments.size() == 0) continue;
        StreetAlgos::calcSegments(joint, listLoadSegments, listLoadJoints);
    }

    VRGeoData* sdata = new VRGeoData();
    VRGeoData* ldata = new VRGeoData();
    VRGeoData* jdata = new VRGeoData();
    VRGeoData* signs2 = new VRGeoData();
    VRAnnotationEnginePtr signs = VRAnnotationEngine::create();
    signs->setSize(Config::get()->SIGN_WIDTH);
    signs->setColor(Vec4f(1,1,1,1));
    signs->setBackground(Vec4f(0.1,0.1,0.8,1));
    signs->setBillboard(true);


    // load street joints
    for (auto jointId : listLoadJoints) {
        StreetJoint* joint = jointId.second;
        if (joint->segments.size() == 0) continue;

        for (auto seg : joint->segments) {
            if (seg->bridge) {
                joint->bridge = true;
                joint->bridgeHeight = seg->bridgeHeight;
                continue;
            }
            joint->bridgeHeight = 0;
            joint->bridge = false;
            break;
        }

        if (joint->segments.size() == 2) joint->type = J1;
        if (joint->segments.size() == 4) {
            vector<int> sLN;
            for (auto seg : joint->segments) sLN.push_back(seg->lanes);
            sort(sLN.begin(), sLN.end());

            if ((sLN[0] == sLN[1]) == (sLN[2] == 1) && sLN[3] == 3) {
                joint->type = J1x3L_3x1L;
            }
        }

        switch (joint->type) {
            case J1: makeCurve(joint, listLoadSegments, listLoadJoints, sdata); break;
            case J1x3L_3x1L:
                makeJoint31(joint, listLoadSegments, listLoadJoints, sdata, signs2); break;
            default: makeJoint(joint, listLoadSegments, listLoadJoints, jdata); break;
        }
    }

    for (auto seg : listLoadSegments) {
        makeSegment(seg.second, listLoadJoints, sdata, signs2); // load street segments
        makeStreetNameSign(seg.second, signs);
        makeStreetLight(seg.second, ldata);
    }

    // init geometries
    auto setGeo = [&](string name, int type, VRGeoData* data, VRMaterialPtr mat) {
        auto geo = data->asGeometry(name);
        geo->setMaterial(mat);
        root->addChild(geo);
        meshes[bbox.str+name] = geo;
    };

    setGeo("streetQuads", GL_QUADS, sdata, matStreet);
    setGeo("streetTriangles", GL_TRIANGLES, jdata, matStreet);
    //setGeo("streetSigns", GL_QUADS, signs2, matSigns);
    setGeo("streetLights", GL_QUADS, ldata, matLights);

    root->addChild(signs);
    annotations[bbox.str+"_signs"] = signs;

    delete sdata;
    delete ldata;
    delete jdata;
    delete signs2;
}

void ModuleStreets::unloadBbox(MapGrid::Box bbox) {
    auto annDestroy = [&](string key) { if (!annotations.count(key)) return; annotations[key]->destroy(); annotations.erase(key); };
    auto resDestroy = [&](string key) { if (!meshes.count(key)) return; meshes[key]->destroy(); meshes.erase(key); };

    annDestroy(bbox.str+"_signs");
    resDestroy(bbox.str+"streetQuads");
    resDestroy(bbox.str+"streetTriangles");
    resDestroy(bbox.str+"streetSigns");
}

void ModuleStreets::physicalize(bool b) {
    for (auto mesh : meshes) {
        mesh.second->getPhysics()->setShape("Concave");
        mesh.second->getPhysics()->setPhysicalized(true);
    }
}

/*
TODO:
- build up asset utility
    - street lamps for example are complex models
    - merge them to a single geometry and store it as template!!
    - duplicate them and append them to the VRGeoData thingy!
*/

void ModuleStreets::makeStreetLight(StreetSegment* seg, VRGeoData* geo) {
    if (seg->name == "") return;
    Vec3f pA = elevate(seg->jointA->position, 0);
    Vec3f pB = elevate(seg->jointB->position, 0);
    Vec3f D = pA-pB;
    float DL = D.length();
    float spread = 9*seg->width;
    if (DL < spread*2) return;

    auto W = seg->leftA - seg->rightA;
    Vec3f X(W[0], 0, W[1]);
    X.normalize();
    X *= seg->width*0.5+0.3;

    int N = floor(DL/spread);
    for (int i=0; i<N; i++) {
        int k = i%2*2-1; // -1 or 1
        pose p(pB+D*i/N-X*k, X*k, Vec3f(0,1,0));
        StreetLamp::add(p, geo);
    }
}

void ModuleStreets::makeStreetSign(Vec3f p, string name, VRGeoData* geo) {
    if (!signTCs.count(name)) return;
    p += Vec3f(0,1.6,0);
    float s = 0.2;
    Vec3f u(0,s,0);
    Vec3f n(0,0,1);
    Vec3f x = u.cross(n);
    pushQuad(p-x-u, p+x-u, p+x+u, p-x+u, n, geo, signTCs[name]);
}

void ModuleStreets::makeStreetNameSign(StreetSegment* seg, VRAnnotationEnginePtr ae) {
    if (!ae) return;
    if (seg->name == "") return;
    if (seg->jointA->type == J1 && seg->jointB->type == J1) return;

    string name = seg->name;
    for (uint i=1; i<name.size(); i++) {
        int n = name[i];
        if (n == -68) { name[i-1] = 'u'; name[i] = 'e'; }
        if (n == -74) { name[i-1] = 'o'; name[i] = 'e'; }
        if (n == -92) { name[i-1] = 'a'; name[i] = 'e'; }
        if (n == -97) { name[i-1] = 's'; name[i] = 's'; }
    }

    float h1 = Config::get()->SIGN_DISTANCE;

    //Vec2f dir = (seg->leftA - seg->rightA); dir.normalize();
    Vec2f p2 = (seg->leftA + seg->leftB)*0.5;
    Vec3f p(p2[0], h1, p2[1]);
    ae->add(p, name);
}

void ModuleStreets::makeSegment(StreetSegment* s, map<string, StreetJoint*>& joints, VRGeoData* streets, VRGeoData* signs) {
    Vec2f leftA = Vec2f(s->leftA);
    Vec2f rightA = Vec2f(s->rightA);
    Vec2f leftB = Vec2f(s->leftB);
    Vec2f rightB = Vec2f(s->rightB);

    if (s->lanes <= 0) s->lanes = 1;
    Vec2f laneW = (rightA-leftA)*1/s->lanes;
    float streetH = Config::get()->STREET_HEIGHT;

    if (!s->bridge) {//normal street
        for (int l = 0; l < s->lanes; l++) {
            auto lA = elevate(leftA+laneW*l, streetH);
            auto lB = elevate(leftB+laneW*l, streetH);
            auto rA = elevate(rightA-laneW*(s->lanes-l-1), streetH);
            auto rB = elevate(rightB-laneW*(s->lanes-l-1), streetH);
            float k1 = 0.75, k2 = 1;
            if (s->lanes > 1) {
                k1 = (l == 0) ? 0 : (l%2) ? 0.25 : 0.5;
                k2 = (l == s->lanes-1) ? (l%2) ? 0 : 0.75 : (l%2) ? 0.5 : 0.25;
            }
            pushStreetQuad( lA, rA, rB, lB, Vec3f(0,1,0), streets, false, Vec3f(k1,k2,1) );
        }
        return;
    }

    // bridge
    Vec2f ABPart = (leftB - leftA)/3;
    float high = streetH + s->bridgeHeight;
    Vec3f th = Vec3f(0, Config::get()->BRIDGE_SIZE, 0);

    Vec3f laneW3 = Vec3f(laneW[0], 0, laneW[1]);
    auto pushPart = [&](Vec2f height, int i) {
        Vec2f lA = leftA + ABPart*i;
        Vec2f rA = rightA + ABPart*i;
        Vec3f a1 = elevate(lA, height[0]);
        Vec3f a2 = elevate(rA, height[0]);
        Vec3f b1 = elevate(lA + ABPart, height[1]);
        Vec3f b2 = elevate(rA + ABPart, height[1]);

        Vec3f normal = (a2-a1).cross(b1-a1);
        for (int l = 0; l < s->lanes; l++) {
            auto lA = a1+laneW3*l;
            auto lB = b1+laneW3*l;
            auto rA = a2-laneW3*(s->lanes-l-1);
            auto rB = b2-laneW3*(s->lanes-l-1);
            float k1 = 0.75, k2 = 1;
            if (s->lanes > 1) {
                k1 = (l == 0) ? 0 : (l%2) ? 0.25 : 0.5;
                k2 = (l == s->lanes-1) ? (l%2) ? 0 : 0.75 : (l%2) ? 0.5 : 0.25;
            }
            pushStreetQuad( lA, rA, rB, lB, -normal, streets, false, Vec3f(k1,k2,1) );
        }
        pushStreetQuad(a1-th, a2-th, b2-th, b1-th, normal, streets, false, Vec3f(0.75,1,1) );
        pushStreetQuad(a1, a1-th, b1-th, b1, Vec3f(-(b1-a1)[2], 0, (b1-a1)[0]), streets, true); //side1
        pushStreetQuad(a2, a2-th, b2-th, b2, Vec3f((b2-a2)[2], 0, -(b2-a2)[0]), streets, true); //side2
    };

    if (!s->jointA || !s->jointB) return;
    Vec4f heights = Vec4f(high, high, high, high);
    if (s->jointA->bridge && !s->jointB->bridge) heights = Vec4f(streetH, high*0.3, high*0.7, high);
    if (!s->jointA->bridge && s->jointB->bridge) heights = Vec4f(high, high*0.7, high*0.3, streetH);

    pushPart(Vec2f(heights[0], heights[1]), 0); // start
    pushPart(Vec2f(heights[1], heights[2]), 1); // middle
    pushPart(Vec2f(heights[2], heights[3]), 2); // end

    // Idee: baue eine maximale rampenlänge ein
    // Idee: berechne Steigung und entscheide dementsprechend fuer Treppen
}

void ModuleStreets::pushQuad(Vec3f a1, Vec3f a2, Vec3f b2, Vec3f b1, Vec3f normal, VRGeoData* geo, Vec4f tc) {
    pushQuad(a1,a2,b2,b1,normal,geo,Vec2f(tc[0], tc[2]), Vec2f(tc[1], tc[2]), Vec2f(tc[1], tc[3]), Vec2f(tc[0], tc[3]));
}

void ModuleStreets::pushStreetQuad(Vec3f a1, Vec3f a2, Vec3f b2, Vec3f b1, Vec3f normal, VRGeoData* geo, bool isSide, Vec3f tc) {
    // calc road length && divide by texture size
    float width = (a2-a1).length();
    float len = (b1 - a1).length()/width*tc[2]; // tc2 is the number of lanes

    Vec2f tc01(0.1, 0.1);
    if (isSide) pushQuad(a1,a2,b2,b1,normal,geo,tc01,tc01,tc01,tc01);
    else pushQuad(a1,a2,b2,b1,normal,geo,Vec2f(tc[0], 0), Vec2f(tc[1], 0), Vec2f(tc[1], len), Vec2f(tc[0], len));
}

void ModuleStreets::pushQuad(Vec3f a1, Vec3f a2, Vec3f b2, Vec3f b1, Vec3f normal, VRGeoData* geo, Vec2f tc1, Vec2f tc2, Vec2f tc3, Vec2f tc4) {
    geo->pushVert(a1, normal, tc1);
    geo->pushVert(a2, normal, tc2);
    geo->pushVert(b2, normal, tc3);
    geo->pushVert(b1, normal, tc4);
    geo->pushQuad();
}

void ModuleStreets::pushTriangle(Vec3f a1, Vec3f a2, Vec3f c, Vec3f normal, VRGeoData* geo, Vec2f t1, Vec2f t2, Vec2f t3 ) {
    geo->pushVert(a1, normal, t1);
    geo->pushVert(a2, normal, t2);
    geo->pushVert(c,  normal, t3);
    geo->pushTri();
}

Vec3f ModuleStreets::elevate(Vec2f p, float h) {
    auto mc = RealWorld::get()->getCoordinator();
    if (!mc) return Vec3f(p[0], 0, p[1]);
    return Vec3f(p[0], mc->getElevation(p) + h, p[1]);
}

void ModuleStreets::makeCurve(StreetJoint* sj, map<string, StreetSegment*>& streets, map<string, StreetJoint*>& joints, VRGeoData* geo) {
    vector<JointPoints*> jointPoints = StreetAlgos::calcJoints(sj, streets, joints);
    float jointH = Config::get()->STREET_HEIGHT + sj->bridgeHeight;

    Vec2f _NULL;
    Vec3f norm = Vec3f(0, 1, 0);

    JointPoints* jp1 = jointPoints[0];
    JointPoints* jp2 = jointPoints[1];
    int Nl1 = max(sj->segments[0]->lanes, 1);
    int Nl2 = max(sj->segments[1]->lanes, 1);
    int Nl = min(Nl1, Nl2);
    if (jp1->leftExt != _NULL && jp1->leftExt != _NULL) {
        Vec3f r1,l1,r2,l2,e1,e2,d1,d2,d3;
        r1 = elevate(jp1->right  , jointH);
        l1 = elevate(jp1->left   , jointH);
        r2 = elevate(jp2->right  , jointH);
        l2 = elevate(jp2->left   , jointH);
        e1 = elevate(jp1->leftExt, jointH);
        e2 = elevate(jp2->leftExt, jointH);
        d1 = (r1-l1)*1.0/Nl; d2 = (l2-r2)*1.0/Nl; d3 = (e2-e1)*1.0/Nl;
        for (int i=0; i<Nl; i++) {
            float k1 = 0.75, k2 = 1;
            if (Nl > 1) {
                k1 = (i == 0) ? 0.75 : (i%2) ? 0.5 : 0.25;
                k2 = (i == Nl-1) ? (i%2) ? 0.75 : 0 : (i%2) ? 0.25 : 0.5;
            }
            int t = -(Nl-i-1);
            pushStreetQuad(r1 + d1*t, l1 + d1*i, e1 + d3*i, e2 + d3*t, norm, geo, false, Vec3f(k2, k1, 1) );
            pushStreetQuad(r2 + d2*i, l2 + d2*t, e2 + d3*t, e1 + d3*i, norm, geo, false, Vec3f(k1, k2, 1) );
        }
    }
}

void ModuleStreets::makeJoint(StreetJoint* sj, map<string, StreetSegment*>& streets, map<string, StreetJoint*>& joints, VRGeoData* geo) {
    vector<JointPoints*> jointPoints = StreetAlgos::calcJoints(sj, streets, joints);

    int Nsegs = sj->segments.size();
    if (Nsegs <= 1) return;
    float jointH = Config::get()->STREET_HEIGHT + sj->bridgeHeight;

    Vec3f right, left, leftExt, rightExt, firstRight, firstLeft, prevLeft, prevRight;
    Vec3f _NULL;
    Vec2f _NULL2;
    Vec3f norm = Vec3f(0, 1, 0);
    Vec3f middle = elevate(sj->position, jointH);
    Vec2f tm = Vec2f(0.875, 0.5);

    Vec3f p1,p2,p3;
    Vec2f t1,t2;
    for (uint i=0; i<jointPoints.size(); i++) {
        JointPoints* jp1 = jointPoints[i];
        JointPoints* jp2 = jointPoints[(i+1)%jointPoints.size()];
        p1 = elevate(jp1->right, jointH);
        p2 = elevate(jp1->left, jointH);
        t1 = Vec2f(0.75, 0);
        t2 = Vec2f(1, 0);
        pushTriangle(p1, p2, middle, norm, geo, t1, t2, tm);
        p1 = elevate(jp1->left, jointH);
        p2 = elevate(jp2->right, jointH);
        t1 = Vec2f(1, 0);
        t2 = Vec2f(1, 1);
        pushTriangle(p1, p2, middle, norm, geo, t1, t2, tm);
    }

    if (!sj->bridge) return;


    // TODO
    rightExt = _NULL;
    firstRight = _NULL;
    jointH -= Config::get()->BRIDGE_SIZE;

    Vec2f tc05(0.5, 0.5);
    middle = elevate(sj->position, jointH);
    for (JointPoints* jp : jointPoints) {
        right = elevate(jp->right, jointH);
        left = elevate(jp->left, jointH);
        leftExt = elevate(jp->leftExt, jointH);

        pushTriangle(right, left, middle, norm, geo, Vec2f(0, 1), Vec2f(1, 1), tc05);
        if ((leftExt-middle).length() < 3) {
            pushTriangle(left, leftExt, middle, norm, geo, Vec2f(1, 1), Vec2f(1, 1), tc05);
            Vec3f normal = Vec3f(-(left-leftExt)[2], 0, (left-leftExt)[0]);
            pushTriangle(left, leftExt, leftExt+Vec3f(0, Config::get()->BRIDGE_SIZE, 0), normal, geo, tc05, tc05, tc05);
            pushTriangle(leftExt+Vec3f(0, Config::get()->BRIDGE_SIZE, 0), left+Vec3f(0, Config::get()->BRIDGE_SIZE, 0), left, normal, geo, tc05, tc05, tc05);
        }

        if (rightExt != _NULL && (rightExt-middle).length() < 3) {
            pushTriangle(right, rightExt, middle, norm, geo, Vec2f(1, 1), Vec2f(1, 1), tc05);
            Vec3f normal = Vec3f((right-rightExt)[2], 0, -(right-rightExt)[0]);
            pushTriangle(right, rightExt, rightExt+Vec3f(0, Config::get()->BRIDGE_SIZE, 0), normal, geo, tc05, tc05, tc05);
            pushTriangle(rightExt+Vec3f(0, Config::get()->BRIDGE_SIZE, 0), right+Vec3f(0, Config::get()->BRIDGE_SIZE, 0), right, normal, geo, tc05, tc05, tc05);
        } else firstRight = right;

        rightExt = leftExt;
    }

    float bS = Config::get()->BRIDGE_SIZE;
    pushTriangle(firstRight, rightExt, middle, norm, geo, Vec2f(1, 1), Vec2f(1, 1), tc05);

    Vec3f normal = Vec3f((firstRight-rightExt)[2], 0, -(firstRight-rightExt)[0]);
    pushTriangle(firstRight, rightExt, rightExt+Vec3f(0, bS, 0), normal, geo, tc05, tc05, tc05);
    pushTriangle(rightExt+Vec3f(0, bS, 0), firstRight+Vec3f(0, bS, 0), firstRight, normal, geo, tc05, tc05, tc05);
}

void ModuleStreets::makeJoint31(StreetJoint* sj, map<string, StreetSegment*>& streets, map<string, StreetJoint*>& joints, VRGeoData* geo, VRGeoData* signs2) {
    vector<JointPoints*> jointPoints = StreetAlgos::calcJoints(sj, streets, joints);
    float jointH = Config::get()->STREET_HEIGHT + sj->bridgeHeight;
    Vec3f n = Vec3f(0, 1, 0);

    int sL3 = 0;
    for (int i=0; i<4; i++) if (sj->segments[i]->lanes == 3) sL3 = i;

    JointPoints* jp3 = jointPoints[sL3];
    Vec3f r3 = elevate(jp3->right, jointH);
    Vec3f l3 = elevate(jp3->left , jointH);
    Vec3f s = elevate(sj->position, jointH) - r3+(r3-l3)*0.5;
    Vec3f s3D = (r3-l3)*1/3.0;
    pushStreetQuad(r3, l3, l3+s, r3+s, n, geo, false, Vec3f(0, 0.75, 3));

    makeStreetSign(elevate(sj->position, jointH), "straightOrRight", signs2);

    for (int i=0; i<3; i++) {
        JointPoints* jp = jointPoints[(sL3+1+i)%4];
        Vec3f pr = elevate(jp->right, jointH);
        Vec3f pl = elevate(jp->left, jointH);
        pushStreetQuad(pr, pl, l3+s+s3D*(i+1), l3+s+s3D*i, n, geo, false, Vec3f(0.75, 1, 1));
    }

    cout << "CREATE JOINT TYPE 31\n";
}





