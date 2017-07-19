#ifndef VRTEXTURE_H_INCLUDED
#define VRTEXTURE_H_INCLUDED

#include <OpenSG/OSGSField.h>
#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGColor.h>

#include "core/objects/object/VRObject.h"
#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class Image; OSG_GEN_CONTAINERPTR(Image);

class VRTexture : public std::enable_shared_from_this<VRTexture> {
    private:
        ImageRecPtr img;
        int internal_format = 0;

    public:
        VRTexture();
        VRTexture(ImageRecPtr img);
        ~VRTexture();

        static VRTexturePtr create();
        static VRTexturePtr create(ImageRecPtr img);
        VRTexturePtr ptr();

        void setImage(ImageRecPtr img);
        void setInternalFormat(int ipf);
        int getInternalFormat();
        size_t getByteSize();
        int getPixelByteN();
        int getPixelByteSize();
        ImageRecPtr getImage();

        void read(string path);
        void write(string path);
        int getChannels();
        Vec3i getSize();
        Color4f getPixel(Vec2d uv);
        Color4f getPixel(Vec3i p);
        void setPixel(Vec3i p, Color4f c);

        void resize(Vec3i size, Vec3i offset);
        void paste(VRTexturePtr other, Vec3i offset);
        void merge(VRTexturePtr other, Vec3d pos);
};

OSG_END_NAMESPACE;

#endif // VRTEXTURE_H_INCLUDED
