#ifndef CONFIG_H
#define	CONFIG_H

#include <OpenSG/OSGVector.h>

using namespace std;

namespace OSG { class SimpleMaterial; }

namespace realworld {
    struct Config {
        float gridSize = 0.002f; //Size of loaded Chunks
        float maxTriangleSize = 15; //maximum size of a side of a tesselated triangle;

        //floor values
        //Terrain && Ground
        //static const float GROUND_LVL = -1.0f; //y value of ground
        float LAYER_DISTANCE = 0.11f; // distance between 2 layers, so that one layer is hidden completely

        //streets
        float STREET_HEIGHT = 0.05f; //street distance from floor
        float STREET_WIDTH = 3.0f; //minimum street width
        float BRIDGE_HEIGHT = 10.0f; //distance between ground && a bridge
        float SMALL_BRIDGE_HEIGHT = 5.0f; //thickness of smaller bridges
        float BRIDGE_SIZE = 0.5f; //thickness of the bridge

        //street signs
        float SIGN_DISTANCE = 1.0f; //distance from ground
        float SIGN_WIDTH = 0.3; //sign width

        //buildings
        float WINDOW_DOOR_WIDTH = 2.5f; //width of a door || window
        float BUILDING_FLOOR_HEIGHT = 3.0f; //height of one level of a building
        int MAX_FLOORS = 4; //how many floors a building can have at max
        int FASSADE_SEGMENTS = 4; // number of segments per fassade

        //trees
        float TREE_WIDTH = 3.0f;
        float TREE_HEIGHT = 5.0f;

        Config();
        static Config* get();
        static void createPhongShader(OSG::SimpleMaterial* mat, bool phong = true);
        static OSG::Vec2f getStartPosition();
    };
}

#endif	/* CONFIG_H */
