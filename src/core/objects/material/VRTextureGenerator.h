#ifndef VRTEXTUREGENERATOR_H_INCLUDED
#define VRTEXTUREGENERATOR_H_INCLUDED

#include <OpenSG/OSGVector.h>

#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

enum GEN_TYPE {
    PERLIN,
    BRICKS,
    LINE,
    FILL
};

class VRTextureGenerator {
    private:
        int width = 128;
        int height = 128;
        int depth = 1;
        bool hasAlpha = false;

        struct Layer {
            GEN_TYPE type;
            float amount = 0;
            Vec3f c31,c32;
            Vec4f c41,c42;
            int Nchannels = 3;
        };

        vector<Layer> layers;
        VRTexturePtr img;

        void applyFill(Vec3f* data, Vec4f c);
        void applyFill(Vec4f* data, Vec4f c);
        void applyLine(Vec3f* data, Vec3f p1, Vec3f p2, Vec4f c, float width);
        void applyLine(Vec4f* data, Vec3f p1, Vec3f p2, Vec4f c, float width);

    public:
        VRTextureGenerator();

        void setSize(Vec3i dim, bool doAlpha = 0);
        void setSize(int w, int h, int d = 1);

        void add(GEN_TYPE type, float amount, Vec3f c1, Vec3f c2);
        void add(string type, float amount, Vec3f c1, Vec3f c2);
        void add(GEN_TYPE type, float amount, Vec4f c1, Vec4f c2);
        void add(string type, float amount, Vec4f c1, Vec4f c2);

        void drawFill(Vec4f c);
        void drawLine(Vec3f p1, Vec3f p2, Vec4f c, float width);

        void clearStage();
        VRTexturePtr compose(int seed);

        VRTexturePtr readSharedMemory(string segment, string object);
};

OSG_END_NAMESPACE;

#endif // VRTEXTUREGENERATOR_H_INCLUDED
