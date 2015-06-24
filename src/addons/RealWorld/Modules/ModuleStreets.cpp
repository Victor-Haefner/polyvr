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
    vector<string> listReloadJoints;

    for (OSMNode* node : osmMap->osmNodes) { // Load StreetJoints
        Vec2f pos = this->mapCoordinator->realToWorld(Vec2f(node->lat, node->lon));
        StreetJoint* joint = new StreetJoint(pos, node->id);
        if (streetJointMap.count(node->id)) {
            listReloadJoints.push_back(node->id);
            streetJointMap[node->id]->merge(joint);
        } else streetJointMap[node->id] = joint;
        listLoadJoints.push_back(node->id);
    }

    StreetSegment* segPrev = NULL; // Load StreetSegments
    for (OSMWay* way : osmMap->osmWays) {
        if (isStreet(way)) { // load street segment
            for (unsigned int i=0; i < way->nodeRefs.size()-1; i++) {
                string nodeId1 = way->nodeRefs[i];
                string nodeId2 = way->nodeRefs[i+1];
                string segId = way->id + "-" + boost::to_string(i);

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
                listLoadSegments.push_back(seg->id);
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

    // unload joints
    for (string jointId : listReloadJoints) {
        if (meshes.count(jointId)) { // delete mesh from scene && from world.meshes
            VRObject* obj = meshes[jointId];
            meshes.erase(jointId);
            delete obj;
        }

        // unload segments attached to this joint
        StreetJoint* joint = streetJointMap[jointId];
        for (string segId : joint->segmentIds) {
            if (meshes.count(segId)) { // delete mesh from scene && from world.meshes
                VRObject* obj = meshes[segId];
                meshes.erase(segId);
                delete obj;

                listLoadSegments.push_back(segId);
            }
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
    }
    for (string jointId: listLoadJoints) {
        StreetJoint* joint = streetJointMap[jointId];
        if (joint->segmentIds.size() == 0) continue;
        StreetAlgos::jointCalculateJointPoints(joint, streetSegmentMap, streetJointMap);
    }


    GeometryData* sdata = new GeometryData();

    // load street joints
    for (string jointId : listLoadJoints) {
        StreetJoint* joint = streetJointMap[jointId];
        if (joint->segmentIds.size() == 0) continue;
        makeStreetJointGeometry(joint, sdata);
    }

    // load street segments
    for (string segId : listLoadSegments) {
        StreetSegment* seg = streetSegmentMap[segId];
        makeStreetSegmentGeometry(seg, sdata);
    }

    VRGeometry* streets = new VRGeometry("streets");
    streets->create(GL_QUADS, sdata->pos, sdata->norms, sdata->inds, sdata->texs);
    streets->setMaterial(matStreet);
    root->addChild(streets);
}

void ModuleStreets::unloadBbox(AreaBoundingBox* bbox) {
    OSMMap* osmMap = mapDB->getMap(bbox->str);
    if (!osmMap) return;

    // TODO: use map->diff() to remove all joints etc in osmMap, which are also available in active bboxes

//            vector<string> listUnloadJoints;
//            vector<string> listUnloadSegments;

    // Load StreetJoints
    BOOST_FOREACH(OSMNode* node, osmMap->osmNodes) {
        string jointId = node->id;

        if (meshes.count(jointId)) {
            // delete mesh from scene && from world.meshes
            VRObject* obj = meshes[jointId];
            meshes.erase(jointId);
            delete obj;
        }

        // unload segments attached to this joint
        if (streetJointMap.count(jointId)) {
            StreetJoint* joint = streetJointMap[jointId];
            streetJointMap.erase(jointId);

            BOOST_FOREACH(string segId, joint->segmentIds) {
                if (meshes.count(segId)) {
                    // delete mesh from scene && from world.meshes
                    VRObject* obj = meshes[segId];
                    meshes.erase(segId);
                    streetSegmentMap.erase(segId);
                    delete obj;
                }
            }
        }
    }
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
    data->pos->addValue(Vec3f(w1.getValues()[0], low, w1.getValues()[1]));
    data->texs->addValue(Vec2f(0, 0));

    data->inds->addValue(data->inds->size());
    data->norms->addValue(Vec3f(1, 0, 0));
    data->pos->addValue(Vec3f(w2.getValues()[0], low, w2.getValues()[1]));
    data->texs->addValue(Vec2f(1, 0));

    data->inds->addValue(data->inds->size());
    data->norms->addValue(Vec3f(1, 0, 0));
    data->pos->addValue(Vec3f(w2.getValues()[0], high, w2.getValues()[1]));
    data->texs->addValue(Vec2f(1, 1));

    data->inds->addValue(data->inds->size());
    data->norms->addValue(Vec3f(1, 0, 0));
    data->pos->addValue(Vec3f(w1.getValues()[0], high, w1.getValues()[1]));
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
        a1 = Vec3f(leftA.getValues()[0], this->mapCoordinator->getElevation(leftA) + low, leftA.getValues()[1]);
        a2 = Vec3f(rightA.getValues()[0], this->mapCoordinator->getElevation(rightA) + low, rightA.getValues()[1]);
        b1 = Vec3f(leftB.getValues()[0], this->mapCoordinator->getElevation(leftB) + high, leftB.getValues()[1]);
        b2 = Vec3f(rightB.getValues()[0], this->mapCoordinator->getElevation(rightB) + high, rightB.getValues()[1]);
        pushQuad(a1, a2, b2, b1, -getNormal3D(a2-a1, b1-a1), streets);
        pushQuad(a1-th, a2-th, b2-th, b1-th, getNormal3D(a2-a1, b1-a1), streets);
        pushQuad(a1, a1-th, b1-th, b1, Vec3f(-(b1-a1).getValues()[2], 0, (b1-a1).getValues()[0]), streets, true); //side1
        pushQuad(a2, a2-th, b2-th, b2, Vec3f((b2-a2).getValues()[2], 0, -(b2-a2).getValues()[0]), streets, true); //side2

        //middle part of bridge
        low = Config::get()->STREET_HEIGHT;

        leftA += ABPart;
        rightA += ABPart;
        leftB = leftA + ABPart;
        rightB = rightA + ABPart;
        a1 = Vec3f(leftA.getValues()[0], this->mapCoordinator->getElevation(leftA) + high, leftA.getValues()[1]);
        a2 = Vec3f(rightA.getValues()[0], this->mapCoordinator->getElevation(rightA) + high, rightA.getValues()[1]);
        b1 = Vec3f(leftB.getValues()[0], this->mapCoordinator->getElevation(leftB) + high, leftB.getValues()[1]);
        b2 = Vec3f(rightB.getValues()[0], this->mapCoordinator->getElevation(rightB) + high, rightB.getValues()[1]);
        pushQuad(a1, a2, b2, b1, Vec3f(0, 1, 0), streets); //top
        pushQuad(a1-th, a2-th, b2-th, b1-th, Vec3f(0, -1, 0), streets); //bottom
        pushQuad(a1, a1-th, b1-th, b1, Vec3f(-(b1-a1).getValues()[2], 0, (b1-a1).getValues()[0]), streets, true); //side1
        pushQuad(a2, a2-th, b2-th, b2, Vec3f((b2-a2).getValues()[2], 0, -(b2-a2).getValues()[0]), streets, true); //side2


        //end part of bridge
        if(s->rightBridge) low = high; //if bridge goes on
        leftA += ABPart;
        rightA += ABPart;
        leftB = leftA + ABPart;
        rightB = rightA + ABPart;
        a1 = Vec3f(leftA.getValues()[0], this->mapCoordinator->getElevation(leftA) + high, leftA.getValues()[1]);
        a2 = Vec3f(rightA.getValues()[0], this->mapCoordinator->getElevation(rightA) + high, rightA.getValues()[1]);
        b1 = Vec3f(s->leftB.getValues()[0], this->mapCoordinator->getElevation(s->leftB) + low, s->leftB.getValues()[1]);
        b2 = Vec3f(s->rightB.getValues()[0], this->mapCoordinator->getElevation(s->rightB) + low, s->rightB.getValues()[1]);
        pushQuad(a1, a2, b2, b1, -getNormal3D(a2-a1, b1-a1), streets);
        pushQuad(a1-th, a2-th, b2-th, b1-th, getNormal3D(a2-a1, b1-a1), streets);
        pushQuad(a1, a1-th, b1-th, b1, Vec3f(-(b1-a1).getValues()[2], 0, (b1-a1).getValues()[0]), streets, true); //side1
        pushQuad(a2, a2-th, b2-th, b2, Vec3f((b2-a2).getValues()[2], 0, -(b2-a2).getValues()[0]), streets, true); //side2
    } else{ //normal street
        pushQuad(Vec3f(leftA.getValues()[0], this->mapCoordinator->getElevation(leftA) + Config::get()->STREET_HEIGHT, leftA.getValues()[1]),
                 Vec3f(rightA.getValues()[0], this->mapCoordinator->getElevation(rightA) + Config::get()->STREET_HEIGHT, rightA.getValues()[1]),
                 Vec3f(rightB.getValues()[0], this->mapCoordinator->getElevation(rightB) + Config::get()->STREET_HEIGHT, rightB.getValues()[1]),
                 Vec3f(leftB.getValues()[0], this->mapCoordinator->getElevation(leftB) + Config::get()->STREET_HEIGHT, leftB.getValues()[1]),
                 Vec3f(0,1,0), streets);
    }
}

Vec3f ModuleStreets::getNormal3D(Vec3f v1, Vec3f v2){
    float v1x = v1.getValues()[0];
    float v1y = v1.getValues()[1];
    float v1z = v1.getValues()[2];
    float v2x = v2.getValues()[0];
    float v2y = v2.getValues()[1];
    float v2z = v2.getValues()[2];

    return Vec3f(v1y*v2z - v1z*v2y,
        v1z*v2x - v1x*v2z,
        v1x*v2y - v1y*v2x);
}


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

void ModuleStreets::makeStreetJointGeometry(StreetJoint* sj, GeometryData* geo) {
    VRGeometry* geom = new VRGeometry("StreetSegment");
    vector<Vec3f> pos;
    vector<Vec3f> norms;
    vector<int> inds;
    vector<Vec2f> texs;

    vector<JointPoints*> jointPoints = StreetAlgos::jointCalculateJointPoints(sj, streetSegmentMap, streetJointMap);

    /* look up, if street joint is part of a bridge */
    float jointHeight = Config::get()->STREET_HEIGHT;
    if(sj->segmentIds.size() > 1){
        StreetSegment* seg1 = streetSegmentMap[sj->segmentIds[0]];
        StreetSegment* seg2 = streetSegmentMap[sj->segmentIds[1]];
        if(seg1->bridge && seg2->bridge) {
            jointHeight += Config::get()->BRIDGE_HEIGHT;
            sj->bridge = true;
            if(seg1->smallBridge || seg2->smallBridge)
                sj->smallBridge = true;
                jointHeight = Config::get()->STREET_HEIGHT + Config::get()->SMALL_BRIDGE_HEIGHT;
        }
    }

    Vec3f middle, right, left, leftExt, rightExt, firstRight, firstLeft, prevLeft;
    Vec3f _NULL;

    int ind = 0;
    middle = Vec3f(sj->position.getValues()[0], this->mapCoordinator->getElevation(sj->position) +  jointHeight, sj->position.getValues()[1]);
    float width = 0;
    float mx, my;

    BOOST_FOREACH(JointPoints* jp, jointPoints) {
        right = Vec3f(jp->right.getValues()[0], this->mapCoordinator->getElevation(jp->right) +jointHeight, jp->right.getValues()[1]);
        left = Vec3f(jp->left.getValues()[0], this->mapCoordinator->getElevation(jp->left) + jointHeight, jp->left.getValues()[1]);
        leftExt = Vec3f(jp->leftExt.getValues()[0], this->mapCoordinator->getElevation(jp->leftExt) + jointHeight, jp->leftExt.getValues()[1]);

        width = (left-right).length();
        mx = 0.5;
        my = ((right + (left-right)/2)-middle).length()/width;

        pos.push_back(right); pos.push_back(left); pos.push_back(middle);

        for (int j=0; j<3; j++) {
            norms.push_back(Vec3f(0, 1, 0));
            inds.push_back(ind++);
        }

        if(sj->segmentIds.size() <= 2){
            texs.push_back(Vec2f(0, 0));
            texs.push_back(Vec2f(1, 0));
            texs.push_back(Vec2f(mx, my));
        } else {
            texs.push_back(Vec2f(0, 1));
            texs.push_back(Vec2f(1, 1));
            texs.push_back(Vec2f(0.5, 1));
        }


        if (sj->segmentIds.size() <= 2) { //joint with only 1 || 2 connecting street segments
            if ((leftExt-middle).length() < 3) {
                for (int j=0; j<3; j++) {
                    norms.push_back(Vec3f(0, 1, 0));
                    inds.push_back(ind++);
                }
                pos.push_back(left);
                pos.push_back(leftExt);
                pos.push_back(middle);
                texs.push_back(Vec2f(1, 0));
                texs.push_back(Vec2f(1, (left-leftExt).length()/width));
                texs.push_back(Vec2f(mx, my));
            }
            if (rightExt != _NULL && (rightExt-middle).length() < 3) {
                for (int j=0; j<3; j++) {
                    norms.push_back(Vec3f(0, 1, 0));
                    inds.push_back(ind++);
                }
                pos.push_back(right);
                pos.push_back(rightExt);
                pos.push_back(middle);
                texs.push_back(Vec2f(0, 0));
                texs.push_back(Vec2f(0, (right-rightExt).length()/width));
                texs.push_back(Vec2f(mx, my));
            }
        } else {   //joint with more than 2 connecting street segments
            if(prevLeft != _NULL) {
                for (int j=0; j<3; j++) {
                    norms.push_back(Vec3f(0, 1, 0));
                    inds.push_back(ind++);
                }
                pos.push_back(right); pos.push_back(prevLeft); pos.push_back(middle);
                texs.push_back(Vec2f(1, 1)); texs.push_back(Vec2f(1, 1)); texs.push_back(Vec2f(0.5, 0.5));
            }
        }

        if(firstRight == _NULL) firstRight = right;
        if(firstLeft == _NULL) firstLeft = left;
        rightExt = leftExt;
        prevLeft = left;
    }

    if(sj->segmentIds.size() <= 2){ //joint with only 1 || 2 connecting street segments
        for (int j=0; j<3; j++) {
            norms.push_back(Vec3f(0, 1, 0));
            inds.push_back(ind++);
        }
        pos.push_back(firstRight);
        pos.push_back(rightExt);
        pos.push_back(middle);
        texs.push_back(Vec2f(0, 0));
        texs.push_back(Vec2f(0, (firstRight-rightExt).length()/width));
        texs.push_back(Vec2f(mx, my));
        //texs.push_back(Vec2f(1, 1)); texs.push_back(Vec2f(1, 1)); texs.push_back(Vec2f(0.5, 0.5));
    } else {   //joint with more than 2 connecting street segments
        for (int j=0; j<3; j++) {
            norms.push_back(Vec3f(0, 1, 0));
            inds.push_back(ind++);
        }
        pos.push_back(firstRight);
        pos.push_back(prevLeft);
        pos.push_back(middle);
        texs.push_back(Vec2f(1, 1));
        texs.push_back(Vec2f(1, 1));
        texs.push_back(Vec2f(0.5, 0.5));
    }



    if (sj->bridge) {
        rightExt = _NULL;
        firstRight = _NULL;
        jointHeight -= Config::get()->BRIDGE_SIZE;

        middle = Vec3f(sj->position.getValues()[0], this->mapCoordinator->getElevation(sj->position) + jointHeight, sj->position.getValues()[1]);
        BOOST_FOREACH(JointPoints* jp, jointPoints) {
            right = Vec3f(jp->right.getValues()[0], this->mapCoordinator->getElevation(jp->right) + jointHeight, jp->right.getValues()[1]);
            left = Vec3f(jp->left.getValues()[0], this->mapCoordinator->getElevation(jp->left) + jointHeight, jp->left.getValues()[1]);
            leftExt = Vec3f(jp->leftExt.getValues()[0], this->mapCoordinator->getElevation(jp->leftExt) + jointHeight, jp->leftExt.getValues()[1]);

            pos.push_back(right); pos.push_back(left); pos.push_back(middle);
            inds.push_back(ind++); inds.push_back(ind++); inds.push_back(ind++);
            norms.push_back(Vec3f(0, -1, 0)); norms.push_back(Vec3f(0, -1, 0)); norms.push_back(Vec3f(0, -1, 0));
            texs.push_back(Vec2f(0, 1)); texs.push_back(Vec2f(1, 1)); texs.push_back(Vec2f(0.5, 0.5));

            if ((leftExt-middle).length() < 3) {
                pos.push_back(left); pos.push_back(leftExt); pos.push_back(middle);
                inds.push_back(ind++); inds.push_back(ind++); inds.push_back(ind++);
                norms.push_back(Vec3f(0, -1, 0)); norms.push_back(Vec3f(0, -1, 0)); norms.push_back(Vec3f(0, -1, 0));
                texs.push_back(Vec2f(1, 1)); texs.push_back(Vec2f(1, 1)); texs.push_back(Vec2f(0.5, 0.5));

                Vec3f normal = Vec3f(-(left-leftExt).getValues()[2], 0, (left-leftExt).getValues()[0]);

                pos.push_back(left); pos.push_back(leftExt); pos.push_back(leftExt+Vec3f(0, Config::get()->BRIDGE_SIZE, 0));
                inds.push_back(ind++); inds.push_back(ind++); inds.push_back(ind++);
                norms.push_back(normal); norms.push_back(normal); norms.push_back(normal);
                texs.push_back(Vec2f(0.5, 0.5)); texs.push_back(Vec2f(0.5, 0.5)); texs.push_back(Vec2f(0.5, 0.5));

                pos.push_back(leftExt+Vec3f(0, Config::get()->BRIDGE_SIZE, 0)); pos.push_back(left+Vec3f(0, Config::get()->BRIDGE_SIZE, 0)); pos.push_back(left);
                inds.push_back(ind++); inds.push_back(ind++); inds.push_back(ind++);
                norms.push_back(normal); norms.push_back(normal); norms.push_back(normal);
                texs.push_back(Vec2f(0.5, 0.5)); texs.push_back(Vec2f(0.5, 0.5)); texs.push_back(Vec2f(0.5, 0.5));
            }

            if (rightExt != _NULL && (rightExt-middle).length() < 3) {
                pos.push_back(right); pos.push_back(rightExt); pos.push_back(middle);
                inds.push_back(ind++); inds.push_back(ind++); inds.push_back(ind++);
                norms.push_back(Vec3f(0, -1, 0)); norms.push_back(Vec3f(0, -1, 0)); norms.push_back(Vec3f(0, -1, 0));
                texs.push_back(Vec2f(1, 1)); texs.push_back(Vec2f(1, 1)); texs.push_back(Vec2f(0.5, 0.5));

                Vec3f normal = Vec3f((right-rightExt).getValues()[2], 0, -(right-rightExt).getValues()[0]);

                pos.push_back(right); pos.push_back(rightExt); pos.push_back(rightExt+Vec3f(0, Config::get()->BRIDGE_SIZE, 0));
                inds.push_back(ind++); inds.push_back(ind++); inds.push_back(ind++);
                norms.push_back(normal); norms.push_back(normal); norms.push_back(normal);
                texs.push_back(Vec2f(0.5, 0.5)); texs.push_back(Vec2f(0.5, 0.5)); texs.push_back(Vec2f(0.5, 0.5));

                pos.push_back(rightExt+Vec3f(0, Config::get()->BRIDGE_SIZE, 0)); pos.push_back(right+Vec3f(0, Config::get()->BRIDGE_SIZE, 0)); pos.push_back(right);
                inds.push_back(ind++); inds.push_back(ind++); inds.push_back(ind++);
                norms.push_back(normal); norms.push_back(normal); norms.push_back(normal);
                texs.push_back(Vec2f(0.5, 0.5)); texs.push_back(Vec2f(0.5, 0.5)); texs.push_back(Vec2f(0.5, 0.5));
            } else firstRight = right;

            rightExt = leftExt;
        }

        pos.push_back(firstRight); pos.push_back(rightExt); pos.push_back(middle);
        inds.push_back(ind++); inds.push_back(ind++); inds.push_back(ind++);
        norms.push_back(Vec3f(0, -1, 0)); norms.push_back(Vec3f(0, -1, 0)); norms.push_back(Vec3f(0, -1, 0));
        texs.push_back(Vec2f(1, 1)); texs.push_back(Vec2f(1, 1)); texs.push_back(Vec2f(0.5, 0.5));

        Vec3f normal = Vec3f((firstRight-rightExt).getValues()[2], 0, -(firstRight-rightExt).getValues()[0]);

        pos.push_back(firstRight); pos.push_back(rightExt); pos.push_back(rightExt+Vec3f(0, Config::get()->BRIDGE_SIZE, 0));
        inds.push_back(ind++); inds.push_back(ind++); inds.push_back(ind++);
        norms.push_back(normal); norms.push_back(normal); norms.push_back(normal);
        texs.push_back(Vec2f(0.5, 0.5)); texs.push_back(Vec2f(0.5, 0.5)); texs.push_back(Vec2f(0.5, 0.5));

        pos.push_back(rightExt+Vec3f(0, Config::get()->BRIDGE_SIZE, 0)); pos.push_back(firstRight+Vec3f(0, Config::get()->BRIDGE_SIZE, 0)); pos.push_back(firstRight);
        inds.push_back(ind++); inds.push_back(ind++); inds.push_back(ind++);
        norms.push_back(normal); norms.push_back(normal); norms.push_back(normal);
        texs.push_back(Vec2f(0.5, 0.5)); texs.push_back(Vec2f(0.5, 0.5)); texs.push_back(Vec2f(0.5, 0.5));
    }
}
