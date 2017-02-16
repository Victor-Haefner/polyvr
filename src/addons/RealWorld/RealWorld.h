#ifndef REALWORLD_H
#define REALWORLD_H
/*
    TODO:
        - look into splitting
            https://github.com/MaZderMind/osm-history-splitter
            http://wiki.openstreetmap.org/wiki/Osmconvert#Clipping_based_on_Longitude_and_Latitude
*/

#include "core/objects/VRObjectFwd.h"
#include "core/objects/object/VRObject.h"
#include "Altitude.h"
#include <map>

class OSMMapDB;

OSG_BEGIN_NAMESPACE;
using namespace std;

class TrafficSimulation;
class MapCoordinator;
class World;
class MapManager;

class RealWorld : public VRObject {
    private:
        MapCoordinator* mapCoordinator = 0;
        World* world = 0;
        MapManager* mapManager = 0;
        OSMMapDB* mapDB = 0;
        TrafficSimulation* trafficSimulation = 0; // Needed for script access
        static map<string, string> options;
        static Altitude altitude; // constructor runs once, single instance
        static RealWorld* singelton;

    public:
        RealWorld(string name);
        ~RealWorld();

        static std::shared_ptr<RealWorld> create(string name);
        static RealWorld* get();

        void init(OSG::Vec2i size);
        void enableModule(string mod, bool b, bool t, bool p);
        void configure(string var, string val);
        static string getOption(string var);
        void update(OSG::Vec3f pos);

        TrafficSimulation* getTrafficSimulation();
        MapCoordinator* getCoordinator();
        MapManager* getManager();
        World* getWorld();
        OSMMapDB* getDB();
};

OSG_END_NAMESPACE;

#endif // REALWORLD_H
