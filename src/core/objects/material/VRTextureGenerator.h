#ifndef VRTEXTUREGENERATOR_H_INCLUDED
#define VRTEXTUREGENERATOR_H_INCLUDED

#include <OpenSG/OSGFieldContainerFields.h>
#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGImage.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

enum NOISE_TYPE {
    PERLIN
};

class VRTextureGenerator {
    private:
        int width = 128;
        int height = 128;

        struct noise_layer {
            NOISE_TYPE noise;
            float amount;
            Vec3f c1,c2;
        };

        vector<noise_layer> layers;

    public:
        VRTextureGenerator();

        void setSize(int w, int h);

        void addNoise(NOISE_TYPE noise, float amount, Vec3f c1, Vec3f c2);
        void clearStage();

        ImageRecPtr compose(int seed);
};

OSG_END_NAMESPACE;

#endif // VRTEXTUREGENERATOR_H_INCLUDED
