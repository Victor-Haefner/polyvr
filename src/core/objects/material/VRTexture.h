#ifndef VRTEXTURE_H_INCLUDED
#define VRTEXTURE_H_INCLUDED

#include <OpenSG/OSGSField.h>
#include "core/math/OSGMathFwd.h"
#include <OpenSG/OSGColor.h>
#include <OpenSG/OSGImage.h>

#include "core/objects/object/VRObject.h"
#include "VRMaterialFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRTexture : public std::enable_shared_from_this<VRTexture> {
    private:
        ImageMTRecPtr img;
        int internal_format = 0;
        VRThreadCbPtr writeWorker;

        void clampToImage(Vec3i& p);

    public:
        VRTexture();
        VRTexture(ImageMTRecPtr img);
        virtual ~VRTexture();

        static VRTexturePtr create();
        static VRTexturePtr create(ImageMTRecPtr img);
        VRTexturePtr ptr();
        VRTexturePtr copy();

        bool read(string path);
        void readGIS(string path);
        void readBuffer(string path, string format, Vec3i layout, int chanels, int Nmipmaps, int internalpf);
        void setImage(ImageMTRecPtr img);
        void setInternalFormat(int ipf);
        void setPixel(Vec3i p, Color4f c);
        void setIthPixel(int i, Color4f c);
        void setByteData(vector<char> data, Vec3i layout, int chanels, int Nmipmaps, int internalpf);
        void setFloatData(vector<float> data, Vec3i layout, int chanels, int Nmipmaps, int internalpf);

        int getInternalFormat();
        size_t getByteSize();
        int getPixelByteN();
        int getPixelByteSize();
        size_t getNPixel();
        vector<Color4f> getPixels(bool invertY = false);
        ImageMTRecPtr getImage();
        vector<Color4f> randomSamples(int N);
        Color4f sampleMeanColor(int N);

        void convertToBytes();
        void writeImage(ImageMTRecPtr img, string path);
        void write(string path, bool doThread = false);
        void writeThreaded(string path, VRTexturePtr self, VRThreadWeakPtr tw);
        int getChannels();
        Vec3i getSize();
        float getAspectRatio();
        Color4f getPixelUV(Vec2d uv, bool invertY = false);
        Color4f getPixelVec(Vec3i p, bool invertY = false);
        Color4f getPixel(int i, bool invertY = false);

        void resize(Vec3i size, bool scale, Vec3i offset);
        void downsize();
        void paste(VRTexturePtr other, Vec3i offset);
        void merge(VRTexturePtr other, Vec3d pos);
        void mixColor(Color4f c, float a);
        void turn(int steps); // turn n times 90 deg

        static string typeToString(int t);
        static string formatToString(int f);
};

OSG_END_NAMESPACE;

#endif // VRTEXTURE_H_INCLUDED
