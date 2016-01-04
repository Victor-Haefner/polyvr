#ifndef VRTEXTUREGENERATOR_H_INCLUDED
#define VRTEXTUREGENERATOR_H_INCLUDED

#include <OpenSG/OSGFieldContainerFields.h>
#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGImage.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

enum GEN_TYPE {
    PERLIN,
    BRICKS
};

class VRTextureGenerator {
    private:
        int width = 128;
        int height = 128;
        int depth = 1;

        struct Layer {
            GEN_TYPE type;
            float amount;
            Vec3f c1,c2;
        };

        vector<Layer> layers;
        ImageRecPtr img;

    public:
        VRTextureGenerator();

        void setSize(Vec3i dim);
        void setSize(int w, int h, int d = 1);

        void add(GEN_TYPE type, float amount, Vec3f c1, Vec3f c2);
        void add(string type, float amount, Vec3f c1, Vec3f c2);
        void clearStage();
        ImageRecPtr compose(int seed);

        ImageRecPtr readSharedMemory(string segment, string object);
};

OSG_END_NAMESPACE;

#endif // VRTEXTUREGENERATOR_H_INCLUDED
