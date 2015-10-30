#include "ModuleStreets.h"

#include "core/objects/material/VRShader.h"
#include "core/objects/geometry/VRPhysics.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/utils/toString.h"
#include "core/utils/VRTimer.h"
#include "core/scene/VRSceneManager.h"
#include "../OSM/OSMMap.h"
#include "../OSM/OSMMapDB.h"
#include "StreetSegment.h"
#include "StreetJoint.h"
#include "../StreetAlgos.h"
#include "../World.h"
#include "../Config.h"
#include "../RealWorld.h"
#include "../MapCoordinator.h"
#include "core/objects/geometry/VRSprite.h"
#include "core/tools/VRAnnotationEngine.h"

#include <boost/exception/to_string.hpp>
#include <OpenSG/OSGGeometry.h>

using namespace OSG;

ModuleStreets::ModuleStreets() : BaseModule("ModuleStreets") {
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
    matStreet->setMagMinFilter("GL_NEAREST", "GL_NEAREST");
    matStreet->setZOffset(-1,-1);

    /*matStreet->addPass();
    matStreet->setWireFrame(true);
    matStreet->setLit(false);
    matStreet->setDiffuse(Vec3f(1,0,0));
    matStreet->setLineWidth(5);*/

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
}

void ModuleStreets::loadBbox(AreaBoundingBox* bbox) {
    auto mapDB = RealWorld::get()->getDB();
    auto mc = RealWorld::get()->getCoordinator();
    OSMMap* osmMap = mapDB->getMap(bbox->str);
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

    GeometryData* sdata = new GeometryData();
    GeometryData* jdata = new GeometryData();
    VRAnnotationEnginePtr signs = VRAnnotationEngine::create();
    signs->setSize(Config::get()->SIGN_WIDTH);
    signs->setColor(Vec4f(1,1,1,1));
    signs->setBackground(Vec4f(0.1,0.1,0.8,1));


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

            if (sLN[0] == sLN[1] == sLN[2] == 1 && sLN[3] == 3) {
                joint->type = J1x3L_3x1L;
            }
        }

        switch (joint->type) {
            case J1: makeCurve(joint, listLoadSegments, listLoadJoints, sdata); break;
            case J1x3L_3x1L: makeJoint31(joint, listLoadSegments, listLoadJoints, sdata); break;
            default: makeJoint(joint, listLoadSegments, listLoadJoints, jdata); break;
        }
    }

    for (auto seg : listLoadSegments) {
        makeSegment(seg.second, listLoadJoints, sdata); // load street segments
        makeSign(seg.second, signs);
    }

    VRGeometryPtr streets = VRGeometry::create("streets");
    VRGeometryPtr joints = VRGeometry::create("joints");
    streets->create(GL_QUADS, sdata->pos, sdata->norms, sdata->inds, sdata->texs);
    joints->create(GL_TRIANGLES, jdata->pos, jdata->norms, jdata->inds, jdata->texs);
    streets->setMaterial(matStreet);
    joints->setMaterial(matStreet);
    root->addChild(streets);
    root->addChild(joints);
    root->addChild(signs);

    meshes[bbox->str+"_streets"] = streets;
    meshes[bbox->str+"_joints"] = joints;
    annotations[bbox->str+"_signs"] = signs;

    delete sdata;
    delete jdata;
}

void ModuleStreets::unloadBbox(AreaBoundingBox* bbox) {
    string sid = bbox->str+"_streets";
    string jid = bbox->str+"_joints";
    string signsid = bbox->str+"_signs";
    if (meshes.count(sid)) { meshes[sid]->destroy(); meshes.erase(sid); }
    if (meshes.count(jid)) { meshes[jid]->destroy(); meshes.erase(jid); }
    if (annotations.count(signsid)) { annotations[signsid]->destroy(); annotations.erase(signsid); }
}

void ModuleStreets::physicalize(bool b) {
    for (auto mesh : meshes) {
        mesh.second->getPhysics()->setShape("Concave");
        mesh.second->getPhysics()->setPhysicalized(true);
    }
}

void ModuleStreets::makeSign(StreetSegment* seg, VRAnnotationEnginePtr ae) {
    if (seg->name == "") return;
    if (seg->jointA->type == J1 && seg->jointB->type == J1) return;

    string name = seg->name;
    for (int i=1; i<name.size(); i++) {
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

void ModuleStreets::makeSegment(StreetSegment* s, map<string, StreetJoint*>& joints, GeometryData* streets) {
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
            pushQuad( lA, rA, rB, lB, Vec3f(0,1,0), streets, false, Vec3f(k1,k2,1) );
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
            pushQuad( lA, rA, rB, lB, -normal, streets, false, Vec3f(k1,k2,1) );
        }
        pushQuad(a1-th, a2-th, b2-th, b1-th, normal, streets, false, Vec3f(0.75,1,1) );
        pushQuad(a1, a1-th, b1-th, b1, Vec3f(-(b1-a1)[2], 0, (b1-a1)[0]), streets, true); //side1
        pushQuad(a2, a2-th, b2-th, b2, Vec3f((b2-a2)[2], 0, -(b2-a2)[0]), streets, true); //side2
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

void ModuleStreets::pushQuad(Vec3f a1, Vec3f a2, Vec3f b2, Vec3f b1, Vec3f normal, GeometryData* geo, bool isSide, Vec3f tc) {
    // calc road length && divide by texture size
    float width = (a2-a1).length();
    float len = (b1 - a1).length()/width*tc[2]; // tc2 is the number of lanes

    geo->pos->addValue(a1);
    geo->pos->addValue(a2);
    geo->pos->addValue(b2);
    geo->pos->addValue(b1);

    int N = geo->inds->size();
    for(int j= 0; j<4; j++) geo->norms->addValue(normal);
    for(int j= 0; j<4; j++) geo->inds->addValue(j+N);

    if (isSide) {
        geo->texs->addValue(Vec2f(0.1, 0.1));
        geo->texs->addValue(Vec2f(0.1, 0.1));
        geo->texs->addValue(Vec2f(0.1, 0.1));
        geo->texs->addValue(Vec2f(0.1, 0.1)); //one color of texture only
    } else {
        geo->texs->addValue(Vec2f(tc[0], 0));
        geo->texs->addValue(Vec2f(tc[1], 0));
        geo->texs->addValue(Vec2f(tc[1], len));
        geo->texs->addValue(Vec2f(tc[0], len));
    }
}

void ModuleStreets::pushTriangle(Vec3f a1, Vec3f a2, Vec3f c, Vec3f normal, GeometryData* geo, Vec2f t1, Vec2f t2, Vec2f t3 ) {
    pushTriangle(a1,a2,c,normal,geo);
    geo->texs->addValue(t1);
    geo->texs->addValue(t2);
    geo->texs->addValue(t3);
}

void ModuleStreets::pushTriangle(Vec3f a1, Vec3f a2, Vec3f c, Vec3f normal, GeometryData* geo) {
    geo->pos->addValue(a1);
    geo->pos->addValue(a2);
    geo->pos->addValue(c);
    int N = geo->inds->size();
    for(int j= 0; j<3; j++) geo->norms->addValue(normal);
    for(int j= 0; j<3; j++) geo->inds->addValue(j+N);
    //geo->texs->addValue(Vec2f(0, 0));
    //geo->texs->addValue(Vec2f(0, 0));
    //geo->texs->addValue(Vec2f(0, 0));
}

Vec3f ModuleStreets::elevate(Vec2f p, float h) {
    auto mc = RealWorld::get()->getCoordinator();
    return Vec3f(p[0], mc->getElevation(p) + h, p[1]);
}

void ModuleStreets::makeCurve(StreetJoint* sj, map<string, StreetSegment*>& streets, map<string, StreetJoint*>& joints, GeometryData* geo) {
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
        float width = d1.length();
        for (int i=0; i<Nl; i++) {
            float k1 = 0.75, k2 = 1;
            if (Nl > 1) {
                k1 = (i == 0) ? 0.75 : (i%2) ? 0.5 : 0.25;
                k2 = (i == Nl-1) ? (i%2) ? 0.75 : 0 : (i%2) ? 0.25 : 0.5;
            }
            int t = -(Nl-i-1);
            pushQuad(r1 + d1*t, l1 + d1*i, e1 + d3*i, e2 + d3*t, norm, geo, false, Vec3f(k2, k1, 1) );
            pushQuad(r2 + d2*i, l2 + d2*t, e2 + d3*t, e1 + d3*i, norm, geo, false, Vec3f(k1, k2, 1) );
        }
    }
}

void ModuleStreets::makeJoint(StreetJoint* sj, map<string, StreetSegment*>& streets, map<string, StreetJoint*>& joints, GeometryData* geo) {
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

    middle = elevate(sj->position, jointH);
    for (JointPoints* jp : jointPoints) {
        right = elevate(jp->right, jointH);
        left = elevate(jp->left, jointH);
        leftExt = elevate(jp->leftExt, jointH);

        pushTriangle(right, left, middle, norm, geo);
        geo->texs->addValue(Vec2f(0, 1)); geo->texs->addValue(Vec2f(1, 1)); geo->texs->addValue(Vec2f(0.5, 0.5));

        if ((leftExt-middle).length() < 3) {
            pushTriangle(left, leftExt, middle, norm, geo);
            geo->texs->addValue(Vec2f(1, 1)); geo->texs->addValue(Vec2f(1, 1)); geo->texs->addValue(Vec2f(0.5, 0.5));

            Vec3f normal = Vec3f(-(left-leftExt)[2], 0, (left-leftExt)[0]);

            pushTriangle(left, leftExt, leftExt+Vec3f(0, Config::get()->BRIDGE_SIZE, 0), normal, geo);
            geo->texs->addValue(Vec2f(0.5, 0.5)); geo->texs->addValue(Vec2f(0.5, 0.5)); geo->texs->addValue(Vec2f(0.5, 0.5));

            pushTriangle(leftExt+Vec3f(0, Config::get()->BRIDGE_SIZE, 0), left+Vec3f(0, Config::get()->BRIDGE_SIZE, 0), left, normal, geo);
            geo->texs->addValue(Vec2f(0.5, 0.5)); geo->texs->addValue(Vec2f(0.5, 0.5)); geo->texs->addValue(Vec2f(0.5, 0.5));
        }

        if (rightExt != _NULL && (rightExt-middle).length() < 3) {
            pushTriangle(right, rightExt, middle, norm, geo);
            geo->texs->addValue(Vec2f(1, 1)); geo->texs->addValue(Vec2f(1, 1)); geo->texs->addValue(Vec2f(0.5, 0.5));

            Vec3f normal = Vec3f((right-rightExt)[2], 0, -(right-rightExt)[0]);

            pushTriangle(right, rightExt, rightExt+Vec3f(0, Config::get()->BRIDGE_SIZE, 0), normal, geo);
            geo->texs->addValue(Vec2f(0.5, 0.5)); geo->texs->addValue(Vec2f(0.5, 0.5)); geo->texs->addValue(Vec2f(0.5, 0.5));

            pushTriangle(rightExt+Vec3f(0, Config::get()->BRIDGE_SIZE, 0), right+Vec3f(0, Config::get()->BRIDGE_SIZE, 0), right, normal, geo);
            geo->texs->addValue(Vec2f(0.5, 0.5)); geo->texs->addValue(Vec2f(0.5, 0.5)); geo->texs->addValue(Vec2f(0.5, 0.5));
        } else firstRight = right;

        rightExt = leftExt;
    }

    Vec2f tc05(0.5, 0.5);
    float bS = Config::get()->BRIDGE_SIZE;
    pushTriangle(firstRight, rightExt, middle, norm, geo, Vec2f(1, 1), Vec2f(1, 1), tc05);

    Vec3f normal = Vec3f((firstRight-rightExt)[2], 0, -(firstRight-rightExt)[0]);
    pushTriangle(firstRight, rightExt, rightExt+Vec3f(0, bS, 0), normal, geo, tc05, tc05, tc05);
    pushTriangle(rightExt+Vec3f(0, bS, 0), firstRight+Vec3f(0, bS, 0), firstRight, normal, geo, tc05, tc05, tc05);
}

void ModuleStreets::makeJoint31(StreetJoint* sj, map<string, StreetSegment*>& streets, map<string, StreetJoint*>& joints, GeometryData* geo) {
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
    pushQuad(r3, l3, l3+s, r3+s, n, geo, false, Vec3f(0, 0.75, 3));

    for (int i=0; i<3; i++) {
        JointPoints* jp = jointPoints[(sL3+1+i)%4];
        Vec3f pr = elevate(jp->right, jointH);
        Vec3f pl = elevate(jp->left, jointH);
        pushQuad(pr, pl, l3+s+s3D*(i+1), l3+s+s3D*i, n, geo, false, Vec3f(0.75, 1, 1));
    }

    cout << "CREATE JOINT TYPE 31\n";
}





