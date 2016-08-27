#ifndef OSMNODE_H
#define OSMNODE_H

#include <string>
#include <vector>
#include <map>

using namespace std;
namespace xmlpp { class Element; }

struct OSMBase {
    string id;
    map<string, string> tags;

    OSMBase(string id);
    OSMBase(xmlpp::Element* e);
};

struct OSMNode : OSMBase {
    double lat = 0;
    double lon = 0;

    OSMNode(string id, double lat, double lon);
    OSMNode(xmlpp::Element* e);
};

struct OSMWay : OSMBase {
    vector<string> nodeRefs;

    OSMWay(string id);
    OSMWay(xmlpp::Element* e);
};

#endif // OSMNODE_H
