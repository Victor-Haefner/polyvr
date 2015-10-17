#ifndef REALWORLD_H
#define REALWORLD_H
/*
    TODO:
        - look into splitting
            https://github.com/MaZderMind/osm-history-splitter
            http://wiki.openstreetmap.org/wiki/Osmconvert#Clipping_based_on_Longitude_and_Latitude
*/

#include "core/objects/VRObjectFwd.h"
#include "Altitude.h"

class TrafficSimulation;
class TextureManager;

OSG_BEGIN_NAMESPACE;
using namespace std;

class MapCoordinator;
class MapGeometryGenerator;
class World;
class MapManager;
class OSMMapDB;
class MapLoader;

class RealWorld {
    private:
        MapCoordinator* mapCoordinator = 0;
        MapGeometryGenerator* mapGeometryGenerator = 0;
        World* world = 0;
        MapManager* mapManager = 0;

        TextureManager* texManager = 0;
        OSMMapDB* mapDB = 0;
        MapLoader* mapLoader = 0;
        static Altitude altitude; // constructor runs once, single instance

        TrafficSimulation *trafficSimulation = 0; // Needed for script access

        bool physicalized;

    public:
        RealWorld(OSG::VRObjectPtr root);
        ~RealWorld();

        void update(OSG::Vec3f pos);

        TrafficSimulation *getTrafficSimulation();

        void physicalize(bool b);
        void enableModule(string mod);
        void disableModule(string mod);
};

OSG_END_NAMESPACE;

#endif // REALWORLD_H
