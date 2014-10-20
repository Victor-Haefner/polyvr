/*
 
 * File is obsolete... keep for stuff in the codes
 
 */

#ifndef WORLDFEATUREEXTRACTOR_H
#define WORLDFEATUREEXTRACTOR_H

#include "../OSM/OSMMap.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

/* Factory for world features */
class WorldFeatureExtractor
{
    public:
        WorldFeatureExtractor() {}

    protected:
    private:

        static void extractFeatureFromWay(OSMMap* map, OSMWay* way) {
            if (way->tags.count("natural")) {
                //printf("--- NATURAL FOUND (%s) \n", way->tags["natural"].c_str());
                return;
            }

            if (way->tags["building"] == "yes") {
                //printf("--- BUILDING FOUND \n");
//                WorldFeature* f = new WorldFeature(new OSMWayGeometry(map, way), "Building");
//                features->push_back(f);
                return;
            }

            if (way->tags.count("highway")) {
                if (way->tags["highway"] == "motorway" ||
                    way->tags["highway"] == "trunk" ||
                    way->tags["highway"] == "primary" ||
                    way->tags["highway"] == "secondary" ||
                    way->tags["highway"] == "tertiary" ||
                    way->tags["highway"] == "living_street" ||
                    way->tags["highway"] == "residential" ||
                    way->tags["highway"] == "unclassified" ||
                    way->tags["highway"] == "road") {


                    //printf("--- STREET FEATURE (%s) FOUND -> CLASS=STREET \n", way->tags["highway"].c_str());
//                    WorldFeature* f = new WorldFeature(new OSMWayGeometry(map, way), "Street");
//                    features->push_back(f);
                    return;
                }
                if (way->tags["highway"] == "pedestrian" ||
                    way->tags["highway"] == "path" ||
                    way->tags["highway"] == "footway" ||
                    way->tags["highway"] == "cycleway") {


                    //printf("--- STREET FEATURE (%s) FOUND -> CLASS=FOOTWAY \n", way->tags["highway"].c_str());
//                    WorldFeature* f = new WorldFeature(new OSMWayGeometry(map, way), "Footway");
//                    features->push_back(f);
                    return;
                }
                //printf("--- STREET FEATURE (%s) FOUND -> CLASS=NONE \n", way->tags["highway"].c_str());
                return;
            }
        }
};

OSG_END_NAMESPACE;

#endif // WORLDFEATUREEXTRACTOR_H
