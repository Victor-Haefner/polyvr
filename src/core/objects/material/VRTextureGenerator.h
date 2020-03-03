#ifndef VRTEXTUREGENERATOR_H_INCLUDED
#define VRTEXTUREGENERATOR_H_INCLUDED

#include "core/math/OSGMathFwd.h"
#include <OpenSG/OSGColor.h>

#include "core/objects/VRObjectFwd.h"
#include "core/math/VRMathFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

enum GEN_TYPE {
    PERLIN,
    BRICKS,
    NORMALMAP,
    LINE,
    FILL,
    PATH,
    POLYGON,
    PIXEL,
    TEXTURE
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
            Color3f c31,c32;
            Color4f c41,c42;
            Vec3i p1;
            PathPtr p;
            VRPolygonPtr pgon;
            VRTexturePtr tex;
            int Nchannels = 3;
        };

        vector<Layer> layers;
        VRTexturePtr img;

        void applyPixel(Color3f* data, Vec3i p, Color4f c);
        void applyPixel(Color4f* data, Vec3i p, Color4f c);
        void applyFill(Color3f* data, Color4f c); // fill does not use template, too slow!
        void applyFill(Color4f* data, Color4f c);
        void applyTexture(Color3f* data, VRTexturePtr t, float amount);
        void applyTexture(Color4f* data, VRTexturePtr t, float amount);
        template<typename T> void applyLine(T* data, Vec3d p1, Vec3d p2, Color4f c, float width);
        template<typename T> void applyPath(T* data, PathPtr p, Color4f c, float width);
        template<typename T> void applyPolygon(T* data, VRPolygonPtr p, Color4f c, float height);

        bool inBox(Pnt3d& p, Vec3d& s);
        Vec3i clamp(Vec3i p);
        Vec3d upscale(Vec3d& p);

    public:
        VRTextureGenerator();
        ~VRTextureGenerator();
        static shared_ptr<VRTextureGenerator> create();

        void set(VRTexturePtr t);
        void setSize(Vec3i dim, bool doAlpha = 0);
        void setSize(int w, int h, int d = 1);
        Vec3i getSize();

        void add(GEN_TYPE type, float amount, Color4f c1, Color4f c2);
        void add(string type, float amount, Color4f c1, Color4f c2);

        void drawFill(Color4f c);
        void drawPixel(Vec3i p, Color4f c);
        void drawLine(Vec3d p1, Vec3d p2, Color4f c, float width);
        void drawPath(PathPtr p, Color4f c, float width);
        void drawPolygon(VRPolygonPtr p, Color4f c, float height = 0);

        void clearStage();
        VRTexturePtr compose(int seed = 0);

        void read(string path);
        VRTexturePtr readSharedMemory(string segment, string object);

        void addSimpleNoise(Vec3i dim, bool doAlpha, Color4f fg, Color4f bg, float amount = 1);
};

OSG_END_NAMESPACE;

#endif // VRTEXTUREGENERATOR_H_INCLUDED
