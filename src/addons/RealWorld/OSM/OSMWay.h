#ifndef OSMWAY_H
#define OSMWAY_H

#include <string>
#include <map>
#include <vector>

using namespace std;

class OSMWay {
    public:
        string id;
        map<string, string> tags;
        vector<string> nodeRefs;

        OSMWay(string id);
};

#endif // OSMWAY_H
