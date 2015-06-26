#include "ModuleStreets.h"

#include "core/objects/material/VRShader.h"
#include "core/objects/geometry/VRPhysics.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/scene/VRSceneManager.h"
#include "../OSM/OSMMap.h"
#include "../OSM/OSMMapDB.h"
#include "../Timer.h"
#include "StreetJoint.h"
#include "StreetSegment.h"
#include "../StreetAlgos.h"
#include "../MapGeometryGenerator.h"

#include <boost/exception/to_string.hpp>
#include <OpenSG/OSGGeometry.h>

using namespace OSG;
using namespace std;
using namespace realworld;

ModuleStreets::ModuleStreets(OSMMapDB* mapDB, MapCoordinator* mapCoordinator, TextureManager* texManager) : BaseModule(mapCoordinator, texManager) {
    this->mapDB = mapDB;
    //this->streetHeight = Config::STREET_HEIGHT + Config::GROUND_LVL;

    // create material
    matStreet = new VRMaterial("Street");
    matStreet->setTexture("world/textures/street1.png");

    matStreet->setAmbient(Color3f(0.5, 0.5, 0.5)); //light reflection in all directions
    matStreet->setDiffuse(Color3f(1.0, 1.0, 1.0)); //light from ambient (without lightsource)
    matStreet->setSpecular(Color3f(0.2, 0.2, 0.2)); //light reflection in camera direction
    string wdir = VRSceneManager::get()->getOriginalWorkdir();
    matStreet->readVertexShader(wdir+"/shader/TexturePhong/phong.vp");
    matStreet->readFragmentShader(wdir+"/shader/TexturePhong/phong.fp");
    matStreet->setMagMinFilter("GL_LINEAR", "GL_NEAREST_MIPMAP_NEAREST");
    matStreet->setZOffset(-1,-1);
    //matStreet->setWireFrame(true);
}

string ModuleStreets::getName() { return "ModuleStreets"; }

bool isStreet(OSMWay* way) {
    return (
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
    OSMMap* osmMap = mapDB->getMap(bbox->str);
    if (!osmMap) return;

    vector<string> listLoadJoints;
    vector<string> listLoadSegments;

    for (OSMNode* node : osmMap->osmNodes) { // Load StreetJoints
        Vec2f pos = this->mapCoordinator->realToWorld(Vec2f(node->lat, node->lon));
        StreetJoint* joint = new StreetJoint(pos, node->id);
        if (streetJointMap.count(node->id)) streetJointMap[node->id]->merge(joint);
        else streetJointMap[node->id] = joint;
        listLoadJoints.push_back(node->id);
    }

    StreetSegment* segPrev = NULL; // Load StreetSegments
    for (OSMWay* way : osmMap->osmWays) {
        if (isStreet(way)) { // load street segment
            for (unsigned int i=0; i < way->nodeRefs.size()-1; i++) {
                string nodeId1 = way->nodeRefs[i];
                string nodeId2 = way->nodeRefs[i+1];
                string segId = way->id + "-" + boost::to_string(i);
                listLoadSegments.push_back(segId);

                if (streetSegmentMap.count(segId)) continue;

                StreetSegment* seg = new StreetSegment(nodeId1, nodeId2, Config::get()->STREET_WIDTH, segId);
                if(way->tags["bridge"] == "yes") {
                    seg->bridge = true;
                    //make all segments of bridge small, if one is small
                    if(seg->getDistance() < Config::get()->BRIDGE_HEIGHT) {
                        seg->smallBridge = true;
                        if (segPrev) if (segPrev->bridge) segPrev->smallBridge = true;
                    } else if(segPrev) {
                        if(segPrev->smallBridge) seg->smallBridge = true;
                    }

                    if(segPrev != NULL) {
                        segPrev->leftBridge = true;
                        if(segPrev->bridge) seg->rightBridge = true;
                    }
                }
                streetSegmentMap[seg->id] = seg;

                if (way->tags.count("lanes")) {
                    seg->lanes = toFloat(way->tags["lanes"].c_str());
                    seg->width = seg->width * seg->lanes;
                } else if (way->tags["highway"] == "secondary") {
                    seg->lanes = 2;
                    seg->width = seg->width * seg->lanes;
                }

                if (way->tags.count("name")) seg->name = way->tags["name"];

                streetJointMap[nodeId1]->segmentIds.push_back(segId);
                streetJointMap[nodeId2]->segmentIds.push_back(segId);
                //listLoadSegments.push_back(seg->id);
                segPrev = seg;
            }
        }
    }

    for (string jointId : listLoadJoints) { // fix up broken segmendIds in joints
        StreetJoint* joint = streetJointMap[jointId];
        for (vector<string>::iterator it = joint->segmentIds.begin(); it != joint->segmentIds.end();) {
            if (!streetSegmentMap.count(*it)) { it = joint->segmentIds.erase(it); cout << "haeh!\n"; }
            else it++;
        }
    }

    // prepare load lists
    StreetAlgos::vectorStrRemoveDuplicates(listLoadJoints);
    StreetAlgos::vectorStrRemoveDuplicates(listLoadSegments);

    // prepare joints
    for (string jointId : listLoadJoints) {
        StreetJoint* joint = streetJointMap[jointId];
        if (joint->segmentIds.size() == 0) continue;
        StreetAlgos::jointCalculateSegmentPoints(joint, streetSegmentMap, streetJointMap);
        StreetAlgos::jointCalculateJointPoints(joint, streetSegmentMap, streetJointMap);
    }

    GeometryData* sdata = new GeometryData();
    GeometryData* jdata = new GeometryData();

    // load street joints
    for (string jointId : listLoadJoints) {
        StreetJoint* joint = streetJointMap[jointId];
        if (joint->segmentIds.size() == 0) continue;
        makeStreetJointGeometry(joint, jdata);
    }

    // load street segments
    for (string segId : listLoadSegments) {
        StreetSegment* seg = streetSegmentMap[segId];
        makeStreetSegmentGeometry(seg, sdata);
    }

    VRGeometry* streets = new VRGeometry("streets");
    VRGeometry* joints = new VRGeometry("joints");
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
}

void ModuleStreets::physicalize(bool b) {
    return;
    for (auto mesh : meshes) {
        mesh.second->getPhysics()->setPhysicalized(true);
        mesh.second->getPhysics()->setShape("Concave");
    }
}

void ModuleStreets::makeSignGeometry(StreetSegment* seg, GeometryData* geo) {
    Vec2f dir = (seg->leftA - seg->leftB)*-1;
    dir.normalize();
    Vec2f dirOrtho = (seg->leftA - seg->rightA);
    dirOrtho.normalize();

    Vec2f w2 = seg->leftA + (dirOrtho*1.5f) + (dir * 1.5f);
    Vec2f w1 = w2 + (dir * 3);
//            Vec2f w3 = w2 - (dirOrtho*0.6f);
//            Vec2f w4 = w1 - (dirOrtho*0.6f);

    float low = this->mapCoordinator->getElevation(w1) + Config::get()->SIGN_DISTANCE;
    float high = low + Config::get()->SIGN_WIDTH;

    GeometryData* data = new GeometryData();
    // create the quad1
    data->inds->addValue(data->inds->size());
    data->norms->addValue(Vec3f(1, 0, 0));
    data->pos->addValue(Vec3f(w1[0], low, w1[1]));
    data->texs->addValue(Vec2f(0, 0));

    data->inds->addValue(data->inds->size());
    data->norms->addValue(Vec3f(1, 0, 0));
    data->pos->addValue(Vec3f(w2[0], low, w2[1]));
    data->texs->addValue(Vec2f(1, 0));

    data->inds->addValue(data->inds->size());
    data->norms->addValue(Vec3f(1, 0, 0));
    data->pos->addValue(Vec3f(w2[0], high, w2[1]));
    data->texs->addValue(Vec2f(1, 1));

    data->inds->addValue(data->inds->size());
    data->norms->addValue(Vec3f(1, 0, 0));
    data->pos->addValue(Vec3f(w1[0], high, w1[1]));
    data->texs->addValue(Vec2f(0, 1));
}

void ModuleStreets::makeStreetSegmentGeometry(StreetSegment* s, GeometryData* streets) {
    Vec2f leftA = Vec2f(s->leftA);
    Vec2f rightA = Vec2f(s->rightA);
    Vec2f leftB = Vec2f(s->leftB);
    Vec2f rightB = Vec2f(s->rightB);

    int ind = 0;

    if (s->bridge){ //bridge
        float low = Config::get()->STREET_HEIGHT;
        float high = Config::get()->STREET_HEIGHT + Config::get()->BRIDGE_HEIGHT;
        Vec3f th = Vec3f(0, Config::get()->BRIDGE_SIZE, 0);
        Vec3f a1, a2, b1, b2;
        int ind = 0;
        Vec2f ABPart = (leftB - leftA)/3;
        if(s->smallBridge) high = Config::get()->STREET_HEIGHT + Config::get()->SMALL_BRIDGE_HEIGHT;

        //start part of bridge
        if(s->leftBridge) low = high;
        leftB = leftA + ABPart;
        rightB = rightA + ABPart;
        a1 = Vec3f(leftA[0], this->mapCoordinator->getElevation(leftA) + low, leftA[1]);
        a2 = Vec3f(rightA[0], this->mapCoordinator->getElevation(rightA) + low, rightA[1]);
        b1 = Vec3f(leftB[0], this->mapCoordinator->getElevation(leftB) + high, leftB[1]);
        b2 = Vec3f(rightB[0], this->mapCoordinator->getElevation(rightB) + high, rightB[1]);
        pushQuad(a1, a2, b2, b1, -getNormal3D(a2-a1, b1-a1), streets);
        pushQuad(a1-th, a2-th, b2-th, b1-th, getNormal3D(a2-a1, b1-a1), streets);
        pushQuad(a1, a1-th, b1-th, b1, Vec3f(-(b1-a1)[2], 0, (b1-a1)[0]), streets, true); //side1
        pushQuad(a2, a2-th, b2-th, b2, Vec3f((b2-a2)[2], 0, -(b2-a2)[0]), streets, true); //side2

        //middle part of bridge
        low = Config::get()->STREET_HEIGHT;

        leftA += ABPart;
        rightA += ABPart;
        leftB = leftA + ABPart;
        rightB = rightA + ABPart;
        a1 = Vec3f(leftA[0], this->mapCoordinator->getElevation(leftA) + high, leftA[1]);
        a2 = Vec3f(rightA[0], this->mapCoordinator->getElevation(rightA) + high, rightA[1]);
        b1 = Vec3f(leftB[0], this->mapCoordinator->getElevation(leftB) + high, leftB[1]);
        b2 = Vec3f(rightB[0], this->mapCoordinator->getElevation(rightB) + high, rightB[1]);
        pushQuad(a1, a2, b2, b1, Vec3f(0, 1, 0), streets); //top
        pushQuad(a1-th, a2-th, b2-th, b1-th, Vec3f(0, -1, 0), streets); //bottom
        pushQuad(a1, a1-th, b1-th, b1, Vec3f(-(b1-a1)[2], 0, (b1-a1)[0]), streets, true); //side1
        pushQuad(a2, a2-th, b2-th, b2, Vec3f((b2-a2)[2], 0, -(b2-a2)[0]), streets, true); //side2


        //end part of bridge
        if(s->rightBridge) low = high; //if bridge goes on
        leftA += ABPart;
        rightA += ABPart;
        leftB = leftA + ABPart;
        rightB = rightA + ABPart;
        a1 = Vec3f(leftA[0], this->mapCoordinator->getElevation(leftA) + high, leftA[1]);
        a2 = Vec3f(rightA[0], this->mapCoordinator->getElevation(rightA) + high, rightA[1]);
        b1 = Vec3f(s->leftB[0], this->mapCoordinator->getElevation(s->leftB) + low, s->leftB[1]);
        b2 = Vec3f(s->rightB[0], this->mapCoordinator->getElevation(s->rightB) + low, s->rightB[1]);
        pushQuad(a1, a2, b2, b1, -getNormal3D(a2-a1, b1-a1), streets);
        pushQuad(a1-th, a2-th, b2-th, b1-th, getNormal3D(a2-a1, b1-a1), streets);
        pushQuad(a1, a1-th, b1-th, b1, Vec3f(-(b1-a1)[2], 0, (b1-a1)[0]), streets, true); //side1
        pushQuad(a2, a2-th, b2-th, b2, Vec3f((b2-a2)[2], 0, -(b2-a2)[0]), streets, true); //side2
    } else{ //normal street
        pushQuad(Vec3f(leftA[0], this->mapCoordinator->getElevation(leftA) + Config::get()->STREET_HEIGHT, leftA[1]),
                 Vec3f(rightA[0], this->mapCoordinator->getElevation(rightA) + Config::get()->STREET_HEIGHT, rightA[1]),
                 Vec3f(rightB[0], this->mapCoordinator->getElevation(rightB) + Config::get()->STREET_HEIGHT, rightB[1]),
                 Vec3f(leftB[0], this->mapCoordinator->getElevation(leftB) + Config::get()->STREET_HEIGHT, leftB[1]),
                 Vec3f(0,1,0), streets);
    }
}

Vec3f ModuleStreets::getNormal3D(Vec3f v1, Vec3f v2) { return v1.cross(v2); }


void ModuleStreets::pushQuad(Vec3f a1, Vec3f a2, Vec3f b2, Vec3f b1, Vec3f normal, GeometryData* geo) {
    pushQuad(a1, a2, b2, b1, normal, geo, false);
}

void ModuleStreets::pushQuad(Vec3f a1, Vec3f a2, Vec3f b2, Vec3f b1, Vec3f normal, GeometryData* geo, bool isSide) {
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
        geo->texs->addValue(Vec2f(0, 0));
        geo->texs->addValue(Vec2f(1, 0));
        geo->texs->addValue(Vec2f(1, len));
        geo->texs->addValue(Vec2f(0, len));
    }
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
    return Vec3f(p[0], mapCoordinator->getElevation(p) + h, p[1]);
}

void ModuleStreets::makeStreetJointGeometry(StreetJoint* sj, GeometryData* geo) {
    vector<JointPoints*> jointPoints = StreetAlgos::jointCalculateJointPoints(sj, streetSegmentMap, streetJointMap);

    int Nsegs = sj->segmentIds.size();

    /* look up, if street joint is part of a bridge */
    float jointHeight = Config::get()->STREET_HEIGHT;
    if (Nsegs > 1) {
        StreetSegment* seg1 = streetSegmentMap[sj->segmentIds[0]];
        StreetSegment* seg2 = streetSegmentMap[sj->segmentIds[1]];
        if (seg1->bridge && seg2->bridge) {
            jointHeight += Config::get()->BRIDGE_HEIGHT;
            sj->bridge = true;
            if (seg1->smallBridge || seg2->smallBridge) {
                sj->smallBridge = true;
                jointHeight = Config::get()->STREET_HEIGHT + Config::get()->SMALL_BRIDGE_HEIGHT;
            }
        }
    }

    Vec3f middle, right, left, leftExt, rightExt, firstRight, firstLeft, prevLeft, prevRight;
    Vec3f _NULL;
    Vec2f _NULL2;
    Vec3f norm = Vec3f(0, 1, 0);

    middle = elevate(sj->position, jointHeight);
    float width = 0;

    vector<Vec3f> fan;
    vector<Vec2f> fantex;

    int i=0;
    for (JointPoints* jp : jointPoints) {
        fan.push_back( elevate(jp->right, jointHeight) );
        fan.push_back( elevate(jp->left, jointHeight) );
        fantex.push_back(Vec2f(0+i%2, i%2));
        fantex.push_back(Vec2f(1-i%2, i%2));
        if (Nsegs <= 2 && jp->leftExt != _NULL2) {
            fan.push_back( elevate(jp->leftExt, jointHeight) );
            fantex.push_back(Vec2f(1-i%2, 0.5));
        }
        i++;
    }

    fan.push_back( elevate(jointPoints[0]->right, jointHeight) ); // close fan
    fantex.push_back(Vec2f(0, 0));

    for (int i=1; i<fan.size(); i++) {
        pushTriangle(fan[i], fan[i-1], middle, norm, geo);
        float my = (Nsegs <= 2) ? 0.5 : fantex[i][1];
        geo->texs->addValue(fantex[i-1]);
        geo->texs->addValue(fantex[i]);
        geo->texs->addValue(Vec2f(0.5, my));
    }

    return; // TODO

    float mx,my;
    if (sj->bridge) {
        rightExt = _NULL;
        firstRight = _NULL;
        jointHeight -= Config::get()->BRIDGE_SIZE;

        middle = elevate(sj->position, jointHeight);
        for (JointPoints* jp : jointPoints) {
            right = elevate(jp->right, jointHeight);
            left = elevate(jp->left, jointHeight);
            leftExt = elevate(jp->leftExt, jointHeight);

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
}
