#ifndef MAPGRID_H_INCLUDED
#define MAPGRID_H_INCLUDED

#include <OpenSG/OSGVector.h>
#include <string>
#include <vector>

OSG_BEGIN_NAMESPACE;
using namespace std;

class MapGrid {
    public:
        struct Box {
            Vec2f min;
            Vec2f max;
            string str;

            Box();
            Box(Vec2f min, float size);

            bool same(Box* b);
        };

    private:
        int dim = 3;
        float size = 1000;
        Vec2f position;
        vector<Box> grid;

        void update();

    public:
        MapGrid(int dim, float size);

        vector<Box>& getBoxes();

        void set(Vec2f p);
        bool has(Box& b);
};

OSG_END_NAMESPACE;

#endif // MAPGRID_H_INCLUDED
