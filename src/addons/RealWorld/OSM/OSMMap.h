#ifndef SIMPLEMAP_H
#define SIMPLEMAP_H

/*

FILE FORMAT INFOS:
    http://wiki.openstreetmap.org/wiki/Elements
    http://wiki.openstreetmap.org/wiki/Map_Features

*/


#include <libxml++/libxml++.h>
#include <boost/foreach.hpp>
#include "OSMNode.h"
#include "OSMWay.h"
#include "core/utils/toString.h"

using namespace OSG;
using namespace std;

namespace realworld {

    class OSMMap
    {
        public:
            int nodeCount;
            int wayCount;
            int areaCount;

            vector<OSMNode*> osmNodes;
            map<string, OSMNode*> osmNodeMap;
            vector<OSMWay*> osmWays;
            float boundsMinLat;
            float boundsMinLon;
            float boundsMaxLat;
            float boundsMaxLon;

            OSMMap(string filepath) {
                this->nodeCount = 0;
                this->wayCount = 0;
                this->areaCount = 0;

                xmlpp::DomParser parser;
                try {
                    parser.parse_file(filepath);
                } catch(const std::exception& ex) {
                    std::cerr << "Exception caught: " << ex.what() << std::endl;
                    return;
                }

                xmlpp::Node::NodeList list = parser.get_document()->get_root_node()->get_children();
                BOOST_FOREACH(xmlpp::Node* node, list) {
                    if (node->get_name() == "node") {
                        readNode(node);
                    } else if (node->get_name() == "way") {
                        readWay(node);
                    } else if (node->get_name() == "bounds") {
                        readBounds(node);
                    }
                }
            }

            static OSMMap* loadMap(string filepath) {
                return new OSMMap(filepath);
            }
        protected:
        private:
            void readNode(xmlpp::Node* node) {
                const xmlpp::Element* element = dynamic_cast<const xmlpp::Element*>(node);
                if (!element) return;

                nodeCount += 1;

                // read attributes
                string id = element->get_attribute_value("id");
                double lat = toFloat(element->get_attribute_value("lat").c_str());
                double lon = toFloat(element->get_attribute_value("lon").c_str());

                // create node and add it to the list
                OSMNode* osmNode = new OSMNode(id, lat, lon);
                osmNodes.push_back(osmNode);
                osmNodeMap[id] = osmNode;

                // read tags of node
                xmlpp::Node::NodeList clist = element->get_children();
                BOOST_FOREACH(xmlpp::Node* cnode, clist) {
                    if (cnode->get_name() == "tag") {
                        readTag(cnode, osmNode->tags);
                    }
                }
            }

            void readWay(xmlpp::Node* node) {
                const xmlpp::Element* element = dynamic_cast<const xmlpp::Element*>(node);
                if (!element) return;

                wayCount += 1;

                // read attributes
                string id = element->get_attribute_value("id");

                // create way and add it to the list
                OSMWay* osmWay = new OSMWay(id);
                osmWays.push_back(osmWay);

                // read tags of way
                xmlpp::Node::NodeList clist = element->get_children();
                BOOST_FOREACH(xmlpp::Node* cnode, clist) {
                    if (cnode->get_name() == "tag") {
                        readTag(cnode, osmWay->tags);
                    } else if (cnode->get_name() == "nd") {
                        readNodeRef(cnode, osmWay->nodeRefs);
                    }
                }
            }

            void readBounds(xmlpp::Node* node) {
                const xmlpp::Element* element = dynamic_cast<const xmlpp::Element*>(node);
                if (!element) return;

                string strMinLat = element->get_attribute_value("minlat");
                string strMinLon = element->get_attribute_value("minlon");
                string strMaxLat = element->get_attribute_value("maxlat");
                string strMaxLon = element->get_attribute_value("maxlon");

                this->boundsMinLat = toFloat(strMinLat.c_str());
                this->boundsMinLon = toFloat(strMinLon.c_str());
                this->boundsMaxLat = toFloat(strMaxLat.c_str());
                this->boundsMaxLon = toFloat(strMaxLon.c_str());
            }

            void readTag(xmlpp::Node* node, map<string, string>& tags) {
                const xmlpp::Element* element = dynamic_cast<const xmlpp::Element*>(node);
                if (!element) return;

                tags[element->get_attribute_value("k")] = element->get_attribute_value("v");
            }

            void readNodeRef(xmlpp::Node* node, vector<string>& nodeRefs) {
                const xmlpp::Element* element = dynamic_cast<const xmlpp::Element*>(node);
                if (!element) return;

                nodeRefs.push_back(element->get_attribute_value("ref"));
            }
    };
};


#endif // SIMPLEMAP_H
