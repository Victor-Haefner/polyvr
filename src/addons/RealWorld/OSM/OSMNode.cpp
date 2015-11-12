#include "OSMNode.h"

OSMNode::OSMNode(string id, double lat, double lon) {
    this->id = id;
    this->lat = lat;
    this->lon = lon;
}

OSMWay::OSMWay(string id) : id(id) {}
