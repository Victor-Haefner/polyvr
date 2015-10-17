#ifndef SIMPLEMAP_H
#define SIMPLEMAP_H

/* FILE FORMAT INFOS:
    http://wiki.openstreetmap.org/wiki/Elements
    http://wiki.openstreetmap.org/wiki/Map_Features
*/

#include <libxml++/libxml++.h>
#include "OSMNode.h"
#include "OSMWay.h"
#include "core/utils/toString.h"

OSG_BEGIN_NAMESPACE;
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
            void readNode(xmlpp::Node* node);
            void readWay(xmlpp::Node* node);
            void readBounds(xmlpp::Node* node);
            void readTag(xmlpp::Node* node, map<string, string>& tags);
            void readNodeRef(xmlpp::Node* node, vector<string>& nodeRefs);
    };

OSG_END_NAMESPACE;

#endif // SIMPLEMAP_H
