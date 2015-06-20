#ifndef REALWORLD_H
#define REALWORLD_H
/*
    TODO:
        - look into splitting
            https://github.com/MaZderMind/osm-history-splitter
            http://wiki.openstreetmap.org/wiki/Osmconvert#Clipping_based_on_Longitude_and_Latitude
*/

#include "Altitude.h"

using namespace std;

namespace OSG{ class VRObject; }

namespace realworld {
    class TrafficSimulation;
    class MapCoordinator;
    class MapGeometryGenerator;
    class World;
    class MapManager;
    class TextureManager;
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
            RealWorld(OSG::VRObject* root);
            ~RealWorld();

            void update(OSG::Vec3f pos);

            TrafficSimulation *getTrafficSimulation();

            void physicalize(bool b);
            void enableModule(string mod);
            void disableModule(string mod);
    };
}



#endif // REALWORLD_H
