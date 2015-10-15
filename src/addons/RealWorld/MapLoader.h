#ifndef MAPLOADER_H
#define	MAPLOADER_H

#include "MapCoordinator.h"
#include "OSM/OSMMap.h"
#include "Modules/StreetJoint.h"
#include "MapData.h"
#include <boost/foreach.hpp>
#include <boost/exception/to_string.hpp>

using namespace OSG;
using namespace std;

namespace realworld {
    class MapLoader {
    public:
        MapCoordinator* mapCoordinator;

        MapLoader(MapCoordinator* mapCoordinator) {
            this->mapCoordinator = mapCoordinator;
        }

        MapData* loadMap(string filename) {

            // check if file exists
            ifstream ifile(filename.c_str());
            if (ifile) ifile.close();
            else return NULL;

            MapData* mapData = new MapData();
            OSMMap* osmMap = OSMMap::loadMap(filename);
            map<string, StreetJoint*> streetJointMap;

            // Load Bounds
            mapData->boundsMin = this->mapCoordinator->realToWorld(Vec2f(osmMap->boundsMinLat, osmMap->boundsMinLon));
            mapData->boundsMax = this->mapCoordinator->realToWorld(Vec2f(osmMap->boundsMaxLat, osmMap->boundsMaxLon));

            // Load StreetJoints
            BOOST_FOREACH(OSMNode* node, osmMap->osmNodes) {
                Vec2f pos = this->mapCoordinator->realToWorld(Vec2f(node->lat, node->lon));
                StreetJoint* joint = new StreetJoint(pos, node->id);
                mapData->streetJoints.push_back(joint);
                joint->info = filename;

                streetJointMap[node->id] = joint;
            }

            // Load StreetSegments && Buildings
            BOOST_FOREACH(OSMWay* way, osmMap->osmWays) {
                if (way->tags["building"] == "yes") {
                    // load building
                    Building* b = new Building(way->id);
                    BOOST_FOREACH(string nodeId, way->nodeRefs) {
                        OSMNode* node = osmMap->osmNodeMap[nodeId];
                        Vec2f pos = this->mapCoordinator->realToWorld(Vec2f(node->lat, node->lon));
                        b->positions.push_back(pos);
                    }
                    mapData->buildings.push_back(b);
                } else if (
                        way->tags["highway"] == "motorway" ||
                        way->tags["highway"] == "trunk" ||
                        way->tags["highway"] == "primary" ||
                        way->tags["highway"] == "secondary" ||
                        way->tags["highway"] == "tertiary" ||
                        way->tags["highway"] == "living_street" ||
                        way->tags["highway"] == "residential" ||
                        way->tags["highway"] == "unclassified" ||
                        way->tags["highway"] == "road") {
                    // load street segment
                    for (unsigned int i=0; i < way->nodeRefs.size()-1; i++) {
                        string nodeId1 = way->nodeRefs[i];
                        string nodeId2 = way->nodeRefs[i+1];
                        string segId = way->id + "-" + boost::to_string(i);

                        StreetSegment* seg = new StreetSegment(nodeId1, nodeId2, 2.1f, segId);
                        mapData->streetSegments.push_back(seg);

                        if (way->tags.count("lanes")) {
                            seg->lanes = toFloat(way->tags["lanes"].c_str());
                            seg->width = seg->width * seg->lanes;
                        } else if (way->tags["highway"] == "secondary") {
                            seg->lanes = 2;
                            seg->width = seg->width * seg->lanes;
                        }

                        if (way->tags.count("name")) {
                            seg->name = way->tags["name"];
                        }

                        streetJointMap[nodeId1]->segmentIds.push_back(segId);
                        streetJointMap[nodeId2]->segmentIds.push_back(segId);
                    }
                }
            }

            return mapData;
        }
    };
}

#endif	/* MAPLOADER_H */

