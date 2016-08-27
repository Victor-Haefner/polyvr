#include "OSMNode.h"
#include "core/utils/toString.h"

#include <libxml++/libxml++.h>

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

OSMNode::OSMNode(xmlpp::Element* el) : OSMBase(el) {
    lat = toFloat(el->get_attribute_value("lat"));
    lon = toFloat(el->get_attribute_value("lon"));
}

OSMWay::OSMWay(xmlpp::Element* el) : OSMBase(el) {
    for(xmlpp::Node* n : el->get_children()) {
        if (auto e = dynamic_cast<xmlpp::Element*>(n)) {
            if (e->get_name() == "nd") {
                nodeRefs.push_back(e->get_attribute_value("ref"));
            }
        }
    }
}

