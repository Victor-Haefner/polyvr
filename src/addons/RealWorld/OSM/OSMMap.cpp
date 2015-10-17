#include "OSMMap.h"

using namespace OSG;

OSMMap::OSMMap(string filepath) {
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
    for(xmlpp::Node* node : list) {
        if (node->get_name() == "node") readNode(node);
        if (node->get_name() == "way") readWay(node);
        if (node->get_name() == "bounds") readBounds(node);
    }
}

OSMMap* OSMMap::loadMap(string filepath) {
    return new OSMMap(filepath);
}

void OSMMap::readNode(xmlpp::Node* node) {
    const xmlpp::Element* element = dynamic_cast<const xmlpp::Element*>(node);
    if (!element) return;

    nodeCount += 1;

    // read attributes
    string id = element->get_attribute_value("id");
    double lat = toFloat(element->get_attribute_value("lat").c_str());
    double lon = toFloat(element->get_attribute_value("lon").c_str());

    // create node && add it to the list
    OSMNode* osmNode = new OSMNode(id, lat, lon);
    osmNodes.push_back(osmNode);
    osmNodeMap[id] = osmNode;

    // read tags of node
    xmlpp::Node::NodeList clist = element->get_children();
    for(xmlpp::Node* cnode : clist) {
        if (cnode->get_name() == "tag") {
            readTag(cnode, osmNode->tags);
        }
    }
}

void OSMMap::readWay(xmlpp::Node* node) {
    const xmlpp::Element* element = dynamic_cast<const xmlpp::Element*>(node);
    if (!element) return;

    wayCount += 1;

    // read attributes
    string id = element->get_attribute_value("id");

    // create way && add it to the list
    OSMWay* osmWay = new OSMWay(id);
    osmWays.push_back(osmWay);

    // read tags of way
    xmlpp::Node::NodeList clist = element->get_children();
    for(xmlpp::Node* cnode : clist) {
        if (cnode->get_name() == "tag") {
            readTag(cnode, osmWay->tags);
        } else if (cnode->get_name() == "nd") {
            readNodeRef(cnode, osmWay->nodeRefs);
        }
    }
}

void OSMMap::readBounds(xmlpp::Node* node) {
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

void OSMMap::readTag(xmlpp::Node* node, map<string, string>& tags) {
    const xmlpp::Element* element = dynamic_cast<const xmlpp::Element*>(node);
    if (!element) return;

    tags[element->get_attribute_value("k")] = element->get_attribute_value("v");
}

void OSMMap::readNodeRef(xmlpp::Node* node, vector<string>& nodeRefs) {
    const xmlpp::Element* element = dynamic_cast<const xmlpp::Element*>(node);
    if (!element) return;

    nodeRefs.push_back(element->get_attribute_value("ref"));
}
