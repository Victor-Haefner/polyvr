#ifndef SIMPLEMAP_H
#define SIMPLEMAP_H

/* FILE FORMAT INFOS:
    http://wiki.openstreetmap.org/wiki/Elements
    http://wiki.openstreetmap.org/wiki/Map_Features
*/

#include "OSMNode.h"

namespace xmlpp { class Element; }
using namespace std;

class OSMMap {
    public:
        int nodeCount = 0;
        int wayCount = 0;
        int areaCount = 0;

        vector<OSMNode*> osmNodes;
        map<string, OSMNode*> osmNodeMap;
        vector<OSMWay*> osmWays;
        float boundsMinLat;
        float boundsMinLon;
        float boundsMaxLat;
        float boundsMaxLon;

        OSMMap(string filepath);

        static OSMMap* loadMap(string filepath);

    private:
        void readNode(xmlpp::Element* element);
        void readWay(xmlpp::Element* element);
        void readBounds(xmlpp::Element* element);
};

#endif // SIMPLEMAP_H
