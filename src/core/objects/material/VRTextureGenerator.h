#ifndef VRTEXTUREGENERATOR_H_INCLUDED
#define VRTEXTUREGENERATOR_H_INCLUDED

#include <OpenSG/OSGFieldContainerFields.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

class Image; OSG_GEN_CONTAINERPTR(Image);

enum NOISE_TYPE {
    PERLIN
};

class VRTextureGenerator {
    private:

    public:
        VRTextureGenerator();

        void setSize(int w, int h);

        void addNoise(NOISE_TYPE noise, float amount);
        void clearStage();

        ImageRecPtr compose(int seed);
};

OSG_END_NAMESPACE;

#endif // VRTEXTUREGENERATOR_H_INCLUDED
