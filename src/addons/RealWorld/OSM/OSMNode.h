#ifndef OSMNODE_H
#define OSMNODE_H

#include <string>
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

#endif // OSMNODE_H
