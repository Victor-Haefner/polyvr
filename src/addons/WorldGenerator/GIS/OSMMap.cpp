#include "OSMMap.h"
#include "core/utils/toString.h"
#include "core/math/boundingbox.h"
#include <libxml++/libxml++.h>

/* FILE FORMAT INFOS:
    http://wiki.openstreetmap.org/wiki/Elements
    http://wiki.openstreetmap.org/wiki/Map_Features
*/

using namespace OSG;

OSMBase::OSMBase(string id) : id(id) {}
OSMNode::OSMNode(string id, double lat, double lon) : OSMBase(id), lat(lat), lon(lon) {}
OSMWay::OSMWay(string id) : OSMBase(id) {}

OSMBase::OSMBase(xmlpp::Element* el) {
    id = el->get_attribute_value("id");
    for(xmlpp::Node* n : el->get_children()) { // read node tags
        if (auto e = dynamic_cast<xmlpp::Element*>(n)) {
            if (e->get_name() == "tag") {
                tags[e->get_attribute_value("k")] = e->get_attribute_value("v");
            }
        }
    }
}

string OSMBase::toString() {
    string res;
    for (auto t : tags) res += " " + t.first + ":" + t.second;
    return res;
}

string OSMNode::toString() {
    string res = OSMBase::toString();
    res += " N" + ::toString(lat) + " E" + ::toString(lon);
    return res;
}

string OSMWay::toString() {
    string res = OSMBase::toString() + " nodes:";
    for (auto n : nodes) res += " " + n;
    return res;
}

OSMNode::OSMNode(xmlpp::Element* el) : OSMBase(el) {
    lat = toFloat(el->get_attribute_value("lat"));
    lon = toFloat(el->get_attribute_value("lon"));
}

OSMWay::OSMWay(xmlpp::Element* el) : OSMBase(el) {
    for(xmlpp::Node* n : el->get_children()) {
        if (auto e = dynamic_cast<xmlpp::Element*>(n)) {
            if (e->get_name() == "nd") {
                nodes.push_back(e->get_attribute_value("ref"));
            }
        }
    }
}

OSMMap::OSMMap(string filepath) {
    bounds = Boundingbox::create();

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

    for (auto way : ways) {
        for (auto nID : way.second->nodes) {
            auto n = getNode(nID);
            way.second->polygon.addPoint(Vec2f(n->lon, n->lat));
        }
    }
}

OSMMapPtr OSMMap::loadMap(string filepath) { return OSMMapPtr( new OSMMap(filepath) ); }
map<string, OSMWayPtr> OSMMap::getWays() { return ways; }
map<string, OSMNodePtr> OSMMap::getNodes() { return nodes; }
OSMNodePtr OSMMap::getNode(string id) { return nodes[id]; }
OSMNodePtr OSMMap::getWay(string id) { return nodes[id]; }

void OSMMap::readNode(xmlpp::Element* element) {
    OSMNodePtr node = OSMNodePtr( new OSMNode(element) );
    nodes[node->id] = node;
}

void OSMMap::readWay(xmlpp::Element* element) {
    OSMWayPtr way = OSMWayPtr( new OSMWay(element) );
    ways[way->id] = way;
}

void OSMMap::readBounds(xmlpp::Element* element) {
    Vec3f min(toFloat( element->get_attribute_value("minlon") ), toFloat( element->get_attribute_value("minlat") ), 0 );
    Vec3f max(toFloat( element->get_attribute_value("maxlon") ), toFloat( element->get_attribute_value("maxlat") ), 0 );
    bounds->clear();
    bounds->update(min);
    bounds->update(max);
}
