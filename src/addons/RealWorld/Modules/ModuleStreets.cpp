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
#include "StreetJoint.h"
#include "StreetSegment.h"
#include "../StreetAlgos.h"
#include "../World.h"
#include "../Config.h"
#include "../RealWorld.h"
#include "../MapCoordinator.h"
#include "core/objects/geometry/VRSprite.h"

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

    matStreet->addPass();
    matStreet->setWireFrame(true);
    matStreet->setLit(false);
    matStreet->setDiffuse(Vec3f(1,0,0));
    matStreet->setLineWidth(5);
}

bool isStreet(OSMWay* way) {
    return (// Idee: return the street width, pair<bool, float>
    way->tags["highway"] == "motorway" ||
    way->tags["highway"] == "trunk" ||
    way->tags["highway"] == "primary" ||
    way->tags["highway"] == "secondary" ||
    way->tags["highway"] == "tertiary" ||
    way->tags["highway"] == "living_street" ||
    way->tags["highway"] == "residential" ||
    way->tags["highway"] == "unclassified" ||
    way->tags["highway"] == "service" ||
    way->tags["highway"] == "motorway_link" ||
    way->tags["highway"] == "trunk_link" ||
    way->tags["highway"] == "primary_link" ||
    way->tags["highway"] == "raceway" ||
    way->tags["highway"] == "cycleway" ||
    way->tags["highway"] == "path" ||
    way->tags["highway"] == "pedestrian" ||
    way->tags["highway"] == "road" ||
    way->tags["highway"] == "bridleway" ||
    way->tags["highway"] == "track" ||
    way->tags["highway"] == "footway" ||
    way->tags["bridge"] == "yes");
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

    StreetSegment* segPrev = NULL; // Load StreetSegments
    for (OSMWay* way : osmMap->osmWays) {
        if (isStreet(way)) { // load street segment
            for (unsigned int i=0; i < way->nodeRefs.size()-1; i++) {
                string nodeId1 = way->nodeRefs[i];
                string nodeId2 = way->nodeRefs[i+1];
                string segId = way->id + "-" + boost::to_string(i);

                StreetSegment* seg = new StreetSegment(nodeId1, nodeId2, Config::get()->STREET_WIDTH, segId);
                if (way->tags["bridge"] == "yes") {
                    seg->bridge = true;
                    //make all segments of bridge small, if one is small
                    if (seg->getDistance() < Config::get()->BRIDGE_HEIGHT) seg->smallBridge = true;
                    if (segPrev) if (segPrev->bridge && seg->smallBridge) segPrev->smallBridge = true;
                    if (segPrev) if (segPrev->smallBridge) seg->smallBridge = true;

                    if (segPrev) {
                        segPrev->leftBridge = true;
                        if (segPrev->bridge) seg->rightBridge = true;
                    }
                }
                listLoadSegments[segId] = seg;

                if (way->tags.count("lanes")) {
                    seg->lanes = toFloat(way->tags["lanes"].c_str());
                    seg->width = seg->width * seg->lanes;
                } else if (way->tags["highway"] == "secondary") {
                    seg->lanes = 2;
                    seg->width = seg->width * seg->lanes;
                }

                if (way->tags.count("name")) seg->name = way->tags["name"];

                listLoadJoints[nodeId1]->segmentIds.push_back(segId);
                listLoadJoints[nodeId2]->segmentIds.push_back(segId);
                segPrev = seg;
            }
        }
    }

    // prepare joints
    for (auto jointId : listLoadJoints) {
        StreetJoint* joint = jointId.second;
        if (joint->segmentIds.size() == 0) continue;
        StreetAlgos::calcSegments(joint, listLoadSegments, listLoadJoints);
    }

    GeometryData* sdata = new GeometryData();
    GeometryData* jdata = new GeometryData();

    // load street joints
    for (auto jointId : listLoadJoints) {
        StreetJoint* joint = jointId.second;
        if (joint->segmentIds.size() == 0) continue;
        if (joint->segmentIds.size() == 2) makeStreetCurveGeometry(joint, listLoadSegments, listLoadJoints, jdata);
        else makeStreetJointGeometry(joint, listLoadSegments, listLoadJoints, jdata);
    }

    for (auto seg : listLoadSegments) {
        makeStreetSegmentGeometry(seg.second, sdata); // load street segments
        auto sign = makeSignGeometry(seg.second);
        if (sign == 0) continue;
        if (signs.count(bbox->str) == 0) signs[bbox->str] = vector<VRGeometryPtr>();
        signs[bbox->str].push_back(sign);
        root->addChild(sign);
    }

    VRGeometryPtr streets = VRGeometry::create("streets");
    VRGeometryPtr joints = VRGeometry::create("joints");
    streets->create(GL_QUADS, sdata->pos, sdata->norms, sdata->inds, sdata->texs);
    joints->create(GL_TRIANGLES, jdata->pos, jdata->norms, jdata->inds, jdata->texs);
    streets->setMaterial(matStreet);
    joints->setMaterial(matStreet);
    root->addChild(streets);
    root->addChild(joints);

    meshes[bbox->str+"_streets"] = streets;
    meshes[bbox->str+"_joints"] = joints;

    delete sdata;
    delete jdata;
}

void ModuleStreets::unloadBbox(AreaBoundingBox* bbox) {
    string sid = bbox->str+"_streets";
    string jid = bbox->str+"_joints";
    if (meshes.count(sid)) { meshes[sid]->destroy(); meshes.erase(sid); }
    if (meshes.count(jid)) { meshes[jid]->destroy(); meshes.erase(jid); }
    if (signs.count(bbox->str)) {
        for (auto s : signs[bbox->str]) s->destroy();
        signs.erase(bbox->str);
    }
}

void ModuleStreets::physicalize(bool b) {
    return;
    for (auto mesh : meshes) {
        mesh.second->getPhysics()->setPhysicalized(true);
        mesh.second->getPhysics()->setShape("Concave");
    }
}

VRGeometryPtr ModuleStreets::makeSignGeometry(StreetSegment* seg) {
    return 0; // TODO
    if (seg->name == "") return 0;

    float h1 = Config::get()->SIGN_DISTANCE;
    float h2 = Config::get()->SIGN_WIDTH;

    Vec2f dir = (seg->leftA - seg->rightA); dir.normalize();
    Vec2f pos = (seg->leftA + seg->leftB)*0.5;

    VRSpritePtr sign = VRSprite::create("sign", false, 3*h2, h2);
    sign->setMaterial(VRMaterial::create("sign"));

    sign->setFontColor(Color4f(1,1,1,1));
    //sign->setLabel(seg->name, 20);
    sign->setLabel("BLABLABLA", 20);

    sign->setFrom( elevate( pos, h1+0.5*h2) );
    sign->setDir( Vec3f(dir[0], 0, dir[1]) );
    sign->getMaterial()->setLit(0);
    return sign;
}

void ModuleStreets::makeStreetSegmentGeometry(StreetSegment* s, GeometryData* streets) {
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
            pushQuad( lA, rA, rB, lB, Vec3f(0,1,0), streets, false, Vec2f(k1,k2) );
        }
        return;
    }

    // bridge
    Vec2f ABPart = (leftB - leftA)/3;
    float high = streetH + Config::get()->BRIDGE_HEIGHT;
    if (s->smallBridge) high = streetH + Config::get()->SMALL_BRIDGE_HEIGHT;
    Vec3f th = Vec3f(0, Config::get()->BRIDGE_SIZE, 0);

    auto pushPart = [&](Vec2f height, int i) {
        Vec2f lA = leftA + ABPart*i;
        Vec2f rA = rightA + ABPart*i;
        Vec3f a1 = elevate(lA, height[0]);
        Vec3f a2 = elevate(rA, height[0]);
        Vec3f b1 = elevate(lA + ABPart, height[1]);
        Vec3f b2 = elevate(rA + ABPart, height[1]);

        Vec3f normal = (a2-a1).cross(b1-a1);
        pushQuad(a1, a2, b2, b1, -normal, streets, false, Vec2f(0.75,1) );
        pushQuad(a1-th, a2-th, b2-th, b1-th, normal, streets, false, Vec2f(0.75,1) );
        pushQuad(a1, a1-th, b1-th, b1, Vec3f(-(b1-a1)[2], 0, (b1-a1)[0]), streets, true); //side1
        pushQuad(a2, a2-th, b2-th, b2, Vec3f((b2-a2)[2], 0, -(b2-a2)[0]), streets, true); //side2
    };

    Vec4f heights = Vec4f(high, high, high, high);
    if (s->leftBridge && !s->rightBridge) heights = Vec4f(high, high*0.7, high*0.3, streetH);
    if (!s->leftBridge && s->rightBridge) heights = Vec4f(streetH, high*0.3, high*0.7, high);

    pushPart(Vec2f(heights[0], heights[1]), 0); // start
    pushPart(Vec2f(heights[1], heights[2]), 1); // middle
    pushPart(Vec2f(heights[2], heights[3]), 2); // end

    // Idee: berechne Steigung und entscheide dementsprechend fuer Treppen
}

void ModuleStreets::pushQuad(Vec3f a1, Vec3f a2, Vec3f b2, Vec3f b1, Vec3f normal, GeometryData* geo, bool isSide, Vec2f tc) {
    // calc road length && divide by texture size
    float width = (a2-a1).length();
    float len = (b1 - a1).length()/width;

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

float ModuleStreets::updateJointBridge(StreetJoint* sj, map<string, StreetSegment*>& streets) { /* look up, if street joint is part of a bridge */
    StreetSegment* seg1 = streets[sj->segmentIds[0]];
    StreetSegment* seg2 = streets[sj->segmentIds[1]];
    sj->bridge = seg1->bridge && seg2->bridge;
    sj->smallBridge = (seg1->smallBridge || seg2->smallBridge) && sj->bridge;
    if (sj->smallBridge) return Config::get()->SMALL_BRIDGE_HEIGHT;
    else if (sj->bridge) return Config::get()->BRIDGE_HEIGHT;
    else return 0;
};

void ModuleStreets::makeStreetCurveGeometry(StreetJoint* sj, map<string, StreetSegment*>& streets, map<string, StreetJoint*>& joints, GeometryData* geo) {
    vector<JointPoints*> jointPoints = StreetAlgos::calcJoints(sj, streets, joints);
    float jointH = Config::get()->STREET_HEIGHT + updateJointBridge(sj, streets);

    Vec2f _NULL;
    Vec3f norm = Vec3f(0, 1, 0);

    JointPoints* jp1 = jointPoints[0];
    JointPoints* jp2 = jointPoints[1];
    int Nl1 = max(streets[sj->segmentIds[0]]->lanes, 1);
    int Nl2 = max(streets[sj->segmentIds[1]]->lanes, 1);
    int Nl = min(Nl1, Nl2);
    if (jp1->leftExt != _NULL && jp1->leftExt != _NULL) {
        Vec3f r1,l1,r2,l2,e1,e2,d1,d2,d3;
        Vec2f tr1,tl1,tr2,tl2,te1,te2;
        r1 = elevate(jp1->right  , jointH);
        l1 = elevate(jp1->left   , jointH);
        r2 = elevate(jp2->right  , jointH);
        l2 = elevate(jp2->left   , jointH);
        e1 = elevate(jp1->leftExt, jointH);
        e2 = elevate(jp2->leftExt, jointH);
        d1 = (r1-l1)*1.0/Nl; d2 = (l2-r2)*1.0/Nl; d3 = (e2-e1)*1.0/Nl;
        float width = d1.length();
        float ta = (e1 - l1).length()/width;
        float te = (r2 - e1).length()/width;
        for (int i=0; i<Nl; i++) {
            float k1 = 0.75, k2 = 1;
            if (Nl > 1) {
                k1 = (i == 0) ? 0.75 : (i%2) ? 0.5 : 0.25;
                k2 = (i == Nl-1) ? (i%2) ? 0.75 : 0 : (i%2) ? 0.25 : 0.5;
            }
            tr1 = Vec2f(k2, ta+te);
            tl1 = Vec2f(k1, ta+te);
            tr2 = Vec2f(k1, 0);
            tl2 = Vec2f(k2, 0);
            te1 = Vec2f(k1, te);
            te2 = Vec2f(k2, te);
            int t = -(Nl-i-1);
            pushTriangle( r1 + d1*t, l1 + d1*i, e1 + d3*i, norm, geo, tr1, tl1, te1 );
            pushTriangle( r1 + d1*t, e1 + d3*i, e2 + d3*t, norm, geo, tr1, te1, te2 );
            pushTriangle( r2 + d2*i, l2 + d2*t, e2 + d3*t, norm, geo, tr2, tl2, te2 );
            pushTriangle( r2 + d2*i, e2 + d3*t, e1 + d3*i, norm, geo, tr2, te2, te1 );
        }
    }
}

void ModuleStreets::makeStreetJointGeometry(StreetJoint* sj, map<string, StreetSegment*>& streets, map<string, StreetJoint*>& joints, GeometryData* geo) {
    vector<JointPoints*> jointPoints = StreetAlgos::calcJoints(sj, streets, joints);

    int Nsegs = sj->segmentIds.size();
    if (Nsegs <= 1) return;
    float jointH = Config::get()->STREET_HEIGHT + updateJointBridge(sj, streets);

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

    //fan.push_back( elevate(jointPoints[0]->right, jointH) ); // close fan
    //fantex.push_back(Vec2f(0.75, 0));


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

    pushTriangle(firstRight, rightExt, middle, norm, geo);
    geo->texs->addValue(Vec2f(1, 1)); geo->texs->addValue(Vec2f(1, 1)); geo->texs->addValue(Vec2f(0.5, 0.5));

    Vec3f normal = Vec3f((firstRight-rightExt)[2], 0, -(firstRight-rightExt)[0]);

    pushTriangle(firstRight, rightExt, rightExt+Vec3f(0, Config::get()->BRIDGE_SIZE, 0), normal, geo);
    geo->texs->addValue(Vec2f(0.5, 0.5)); geo->texs->addValue(Vec2f(0.5, 0.5)); geo->texs->addValue(Vec2f(0.5, 0.5));

    pushTriangle(rightExt+Vec3f(0, Config::get()->BRIDGE_SIZE, 0), firstRight+Vec3f(0, Config::get()->BRIDGE_SIZE, 0), firstRight, normal, geo);
    geo->texs->addValue(Vec2f(0.5, 0.5)); geo->texs->addValue(Vec2f(0.5, 0.5)); geo->texs->addValue(Vec2f(0.5, 0.5));
}
