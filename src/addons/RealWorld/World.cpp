#include "World.h"


        /*1111111111111111
        void unloadMapData(MapData* mapData) {
            BOOST_FOREACH(StreetJoint* j, mapData->streetJoints) {
                if (meshes.count(j->id)) {
                    listUnloadJointMeshes.push_back(j->id);
                }
            }
            BOOST_FOREACH(StreetSegment* seg, mapData->streetSegments) {
                if (meshes.count(seg->id)) {
                    listUnloadSegmentMeshes.push_back(seg->id);
                }
            }
            BOOST_FOREACH(Building* b, mapData->buildings) {
                if (meshes.count(b->id)) {
                    listUnloadBuildingMeshes.push_back(b->id);
                }
            }
        }

        void removeMapData(MapData* mapData) {
            // TODO: deletes to free memory ??
            BOOST_FOREACH(StreetJoint* j, mapData->streetJoints) {
                if (streetJoints.count(j->id)) {
                    streetJoints.erase(j->id);
                }
            }
            BOOST_FOREACH(StreetSegment* seg, mapData->streetSegments) {
                if (streetSegments.count(seg->id)) {
                    streetSegments.erase(seg->id);
                }
            }
            BOOST_FOREACH(Building* b, mapData->buildings) {
                if (buildings.count(b->id)) {
                    buildings.erase(b->id);
                }
            }
        }
        void addMapData(MapData* mapData) {
            BOOST_FOREACH(StreetJoint* j, mapData->streetJoints) {
                if (this->streetJoints.find(j->id) == this->streetJoints.end()) {
                    this->streetJoints[j->id] = j;
                } else {
                    this->streetJoints[j->id]->merge(j);
                }
                if (meshes.count(j->id)) listUnloadJointMeshes.push_back(j->id);
                this->listLoadJointMeshes.push_back(j->id);
            }

            BOOST_FOREACH(StreetSegment* seg, mapData->streetSegments) {
                this->streetSegments[seg->id] = seg;
                if (meshes.count(seg->id)) listUnloadSegmentMeshes.push_back(seg->id);
                this->listLoadSegmentMeshes.push_back(seg->id);
            }

            // add buildings
            BOOST_FOREACH(Building* b, mapData->buildings) {
                this->buildings[b->id] = b;
                if (meshes.count(b->id)) listUnloadBuildingMeshes.push_back(b->id);
                this->listLoadBuildingMeshes.push_back(b->id);
            }

            // add areas
            this->listLoadAreaMeshes.push_back(mapData);
        }*/
