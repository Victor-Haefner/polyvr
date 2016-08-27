#include "OSMMap.h"
#include "core/utils/toString.h"

#include <libxml++/libxml++.h>

using namespace OSG;
using namespace std;

OSMMap::OSMMap(string filepath) {
    xmlpp::DomParser parser;
    try { parser.parse_file(filepath); }
    catch(const exception& ex) { cout << "OSMMap Error: " << ex.what() << endl; return; }

    auto nodes = parser.get_document()->get_root_node()->get_children();
    for (auto node : nodes) {
        if (auto element = dynamic_cast<xmlpp::Element*>(node)) {
            if (element->get_name() == "node") readNode(element);
            if (element->get_name() == "way") readWay(element);
            if (element->get_name() == "bounds") readBounds(element);
        }
    }
}

OSMMap* OSMMap::loadMap(string filepath) { return new OSMMap(filepath); }

void OSMMap::readNode(xmlpp::Element* element) {
    nodeCount++;
    OSMNode* osmn = new OSMNode(element);
    osmNodes.push_back(osmn);
    osmNodeMap[osmn->id] = osmn;
}

void OSMMap::readWay(xmlpp::Element* element) {
    wayCount++;
    OSMWay* osmWay = new OSMWay(element);
    osmWays.push_back(osmWay);
}

void OSMMap::readBounds(xmlpp::Element* element) {
    this->boundsMinLat = toFloat( element->get_attribute_value("minlat") );
    this->boundsMinLon = toFloat( element->get_attribute_value("minlon") );
    this->boundsMaxLat = toFloat( element->get_attribute_value("maxlat") );
    this->boundsMaxLon = toFloat( element->get_attribute_value("maxlon") );
}
