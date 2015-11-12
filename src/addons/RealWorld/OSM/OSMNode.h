#ifndef OSMNODE_H
#define OSMNODE_H

#include <string>
#include <vector>
#include <map>

using namespace std;

class OSMNode {
    public:
        string id;
        double lat;
        double lon;

        map<string, string> tags;

        OSMNode(string id, double lat, double lon);
};

class OSMWay {
    public:
        string id;
        map<string, string> tags;
        vector<string> nodeRefs;

        OSMWay(string id);
};

#endif // OSMNODE_H
