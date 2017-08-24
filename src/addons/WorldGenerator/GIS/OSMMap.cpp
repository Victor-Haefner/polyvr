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
OSMRelation::OSMRelation(string id) : OSMBase(id) {}

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

string OSMRelation::toString() {
    string res = OSMBase::toString();
    return res;
}

OSMNode::OSMNode(xmlpp::Element* el) : OSMBase(el) {
    lat = toFloat(el->get_attribute_value("lat"));
    lon = toFloat(el->get_attribute_value("lon"));
}

OSMWay::OSMWay(xmlpp::Element* el) : OSMBase(el) {
    for(xmlpp::Node* n : el->get_children()) {
        if (auto e = dynamic_cast<xmlpp::Element*>(n)) {
            if (e->get_name() == "tag") continue;
            if (e->get_name() == "nd") {
                nodes.push_back(e->get_attribute_value("ref"));
                continue;
            }
            cout << " OSMWay::OSMWay, unhandled element: " << e->get_name() << endl;
        }
    }
}

OSMRelation::OSMRelation(xmlpp::Element* el) : OSMBase(el) {
    for(xmlpp::Node* n : el->get_children()) {
        if (auto e = dynamic_cast<xmlpp::Element*>(n)) {
            if (e->get_name() == "tag") continue;
            if (e->get_name() == "member") {
                string type = e->get_attribute_value("type");
                if (type == "way") ways.push_back(e->get_attribute_value("ref"));
                if (type == "node") nodes.push_back(e->get_attribute_value("ref"));
                continue;
            }
            cout << " OSMRelation::OSMRelation, unhandled element: " << e->get_name() << endl;
        }
    }
}

OSMMap::OSMMap(string filepath) {
    readFile(filepath);
}

void OSMMap::clear() {
    bounds->clear();
    ways.clear();
    nodes.clear();
}

void OSMMap::readFile(string path) {
    filepath = path;
    bounds = Boundingbox::create();

    xmlpp::DomParser parser;
    try { parser.parse_file(filepath); }
    catch(const exception& ex) { cout << "OSMMap Error: " << ex.what() << endl; return; }

    auto nodes = parser.get_document()->get_root_node()->get_children();
    for (auto node : nodes) {
        if (auto element = dynamic_cast<xmlpp::Element*>(node)) {
            if (element->get_name() == "node") { readNode(element); continue; }
            if (element->get_name() == "way") { readWay(element); continue; }
            if (element->get_name() == "bounds") { readBounds(element); continue; }
            if (element->get_name() == "relation") { readRelation(element); continue; }
            cout << " OSMMap::readFile, unhandled element: " << element->get_name() << endl;
        }
    }

    for (auto way : ways) {
        for (auto nID : way.second->nodes) {
            auto n = getNode(nID);
            way.second->polygon.addPoint(Vec2d(n->lon, n->lat));
            n->Nways++;
        }
    }
}

OSMMapPtr OSMMap::loadMap(string filepath) { return OSMMapPtr( new OSMMap(filepath) ); }
map<string, OSMWayPtr> OSMMap::getWays() { return ways; }
map<string, OSMNodePtr> OSMMap::getNodes() { return nodes; }
map<string, OSMRelationPtr> OSMMap::getRelations() { return relations; }
OSMNodePtr OSMMap::getNode(string id) { return nodes[id]; }
OSMWayPtr OSMMap::getWay(string id) { return ways[id]; }
OSMRelationPtr OSMMap::getRelation(string id) { return relations[id]; }
void OSMMap::reload() { clear(); readFile(filepath); }

void OSMMap::readNode(xmlpp::Element* element) {
    OSMNodePtr node = OSMNodePtr( new OSMNode(element) );
    nodes[node->id] = node;
}

void OSMMap::readWay(xmlpp::Element* element) {
    OSMWayPtr way = OSMWayPtr( new OSMWay(element) );
    ways[way->id] = way;
}

void OSMMap::readRelation(xmlpp::Element* element) {
    OSMRelationPtr rel = OSMRelationPtr( new OSMRelation(element) );
    relations[rel->id] = rel;
}

void OSMMap::readBounds(xmlpp::Element* element) {
    Vec3d min(toFloat( element->get_attribute_value("minlon") ), toFloat( element->get_attribute_value("minlat") ), 0 );
    Vec3d max(toFloat( element->get_attribute_value("maxlon") ), toFloat( element->get_attribute_value("maxlat") ), 0 );
    bounds->clear();
    bounds->update(min);
    bounds->update(max);
}
