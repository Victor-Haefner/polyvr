#ifndef OSMMAP_H
#define OSMMAP_H

#include "GISFwd.h"
#include "core/math/VRMathFwd.h"
#include "core/math/polygon.h"
#include <string>
#include <map>
#include <vector>
#include <OpenSG/OSGConfig.h>


namespace xmlpp { class Element; }
using namespace std;

OSG_BEGIN_NAMESPACE;

class OSMSAXHandler;

class OSMSAXParser;

struct OSMBase {
    string id;
    map<string, string> tags;

    OSMBase(string id);
    OSMBase(xmlpp::Element* e);
    virtual string toString();

    bool hasTag(const string& t);
};

struct OSMNode : OSMBase {
    double lat = 0;
    double lon = 0;
    vector<string> ways;

    OSMNode(string id, double lat, double lon);
    OSMNode(xmlpp::Element* e);
    string toString();
};

struct OSMWay : OSMBase {
    vector<string> nodes;
    VRPolygon polygon;

    OSMWay(string id);
    OSMWay(xmlpp::Element* e, map<string, bool>& invalidIDs);
    string toString();
};

struct OSMRelation : OSMBase {
    vector<string> ways;
    vector<string> nodes;

    OSMRelation(string id);
    OSMRelation(xmlpp::Element* e, map<string, bool>& invalidIDs);
    string toString();
};

class OSMMap {
    private:
        string filepath;
        string newfilepath;
        BoundingboxPtr bounds;
        map<string, OSMWayPtr> ways;
        map<string, OSMNodePtr> nodes;
        map<string, OSMRelationPtr> relations;
        map<string, bool> invalidElements;

        bool isValid(xmlpp::Element* e);
        void readNode(xmlpp::Element* element);
        void readWay(xmlpp::Element* element, map<string, bool>& invalidIDs);
        void readBounds(xmlpp::Element* element);
        void readRelation(xmlpp::Element* element, map<string, bool>& invalidIDs);

        void readFile(string path);
        int readFileStreaming(string path);
        int readFileStreaming(string path, map<string, string> whitelist);

    public:
        OSMMap(string filepath);
        OSMMap(string filepath, bool stream);
        OSMMap(string filepath, bool stream, map<string, string> whitelist);
        static OSMMapPtr loadMap(string filepath);
        static OSMMapPtr parseMap(string filepath);
        static OSMMapPtr shrinkMap(string filepath, string newfilepath, map<string, string> whitelist);

        void clear();
        void reload();

        map<string, OSMWayPtr> getWays();
        map<string, OSMNodePtr> getNodes();
        map<string, OSMRelationPtr> getRelations();
        OSMNodePtr getNode(string id);
        OSMWayPtr getWay(string id);
        OSMRelationPtr getRelation(string id);

        double getMemoryConsumption();

        vector<OSMWayPtr> splitWay(OSMWayPtr way, int segN);
};

OSG_END_NAMESPACE;

#endif // OSMMAP_H
