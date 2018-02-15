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

bool OSMBase::hasTag(const string& t) {
    return tags.count(t) > 0;
}

OSMNode::OSMNode(xmlpp::Element* el) : OSMBase(el) {
    toValue(el->get_attribute_value("lat"), lat);
    toValue(el->get_attribute_value("lon"), lon);
}

OSMWay::OSMWay(xmlpp::Element* el, map<string, bool>& invalidIDs) : OSMBase(el) {
    for(xmlpp::Node* n : el->get_children()) {
        if (auto e = dynamic_cast<xmlpp::Element*>(n)) {
            if (e->get_name() == "tag") continue;
            if (e->get_name() == "nd") {
                string nID = e->get_attribute_value("ref");
                if (invalidIDs.count(nID)) continue;
                nodes.push_back(nID);
                continue;
            }
            cout << " OSMWay::OSMWay, unhandled element: " << e->get_name() << endl;
        }
    }
}

OSMRelation::OSMRelation(xmlpp::Element* el, map<string, bool>& invalidIDs) : OSMBase(el) {
    for(xmlpp::Node* n : el->get_children()) {
        if (auto e = dynamic_cast<xmlpp::Element*>(n)) {
            if (e->get_name() == "tag") continue;
            if (e->get_name() == "member") {
                string type = e->get_attribute_value("type");
                string eID = e->get_attribute_value("ref");
                if (invalidIDs.count(eID)) continue;
                if (type == "way") ways.push_back(eID);
                if (type == "node") nodes.push_back(eID);
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

bool OSMMap::isValid(xmlpp::Element* e) {
    if (e->get_attribute("action")) {
        if (e->get_attribute_value("action") == "delete") {
            invalidElements[e->get_attribute_value("id")] = true;
            return false;
        }
    }
    return true;
};

void OSMMap::readFile(string path) {
    filepath = path;
    bounds = Boundingbox::create();

    xmlpp::DomParser parser;
    try { parser.parse_file(filepath); }
    catch(const exception& ex) { cout << "OSMMap Error: " << ex.what() << endl; return; }

    auto data = parser.get_document()->get_root_node()->get_children();
    for (auto enode : data) {
        if (auto element = dynamic_cast<xmlpp::Element*>(enode)) {
            if (!isValid(element)) continue;
            if (element->get_name() == "node") { readNode(element); continue; }
            if (element->get_name() == "bounds") { readBounds(element); continue; }
            //cout << " OSMMap::readFile, unhandled element: " << element->get_name() << endl;
        }
    }
    for (auto enode : data) {
        if (auto element = dynamic_cast<xmlpp::Element*>(enode)) {
            if (!isValid(element)) continue;
            if (element->get_name() == "way") { readWay(element, invalidElements); continue; }
            //cout << " OSMMap::readFile, unhandled element: " << element->get_name() << endl;
        }
    }
    for (auto enode : data) {
        if (auto element = dynamic_cast<xmlpp::Element*>(enode)) {
            if (!isValid(element)) continue;
            if (element->get_name() == "relation") { readRelation(element, invalidElements); continue; }
            //cout << " OSMMap::readFile, unhandled element: " << element->get_name() << endl;
        }
    }

    for (auto way : ways) {
        for (auto nID : way.second->nodes) {
            auto n = getNode(nID);
            if (!n) { cout << " Error in OSMMap::readFile: no node with ID " << nID << endl; continue; }
            way.second->polygon.addPoint(Vec2d(n->lon, n->lat));
            n->ways.push_back(way.second->id);
        }
    }

    cout << "OSMMap::readFile path " << path << endl;
    cout << "  loaded " << ways.size() << " ways, " << nodes.size() << " nodes and " << relations.size() << " relations" << endl;
}


template <class Key, class Value>
unsigned long mapSize(const std::map<Key,Value> &map){
    unsigned long size = sizeof(map);
    for(typename std::map<Key,Value>::const_iterator it = map.begin(); it != map.end(); ++it){
        size += it->first.capacity();
        size += sizeof(it->second);
    }
    return size;
}

double OSMMap::getMemoryConsumption() {
    double res = 0;

    res += filepath.capacity();
    res += sizeof(*bounds);
    res += mapSize(ways);
    res += mapSize(nodes);
    res += mapSize(relations);
    res += mapSize(invalidElements);

    return res/1048576.0;
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

void OSMMap::readWay(xmlpp::Element* element, map<string, bool>& invalidIDs) {
    OSMWayPtr way = OSMWayPtr( new OSMWay(element, invalidIDs) );
    ways[way->id] = way;
}

void OSMMap::readRelation(xmlpp::Element* element, map<string, bool>& invalidIDs) {
    OSMRelationPtr rel = OSMRelationPtr( new OSMRelation(element, invalidIDs) );
    relations[rel->id] = rel;
}

void OSMMap::readBounds(xmlpp::Element* element) {
    Vec3d min(toFloat( element->get_attribute_value("minlon") ), toFloat( element->get_attribute_value("minlat") ), 0 );
    Vec3d max(toFloat( element->get_attribute_value("maxlon") ), toFloat( element->get_attribute_value("maxlat") ), 0 );
    bounds->clear();
    bounds->update(min);
    bounds->update(max);
}
