#ifndef VRTEXTURE_H_INCLUDED
#define VRTEXTURE_H_INCLUDED

#include <OpenSG/OSGSField.h>

#include "core/objects/object/VRObject.h"
#include "core/objects/VRObjectFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class Image; OSG_GEN_CONTAINERPTR(Image);

class VRTexture : public std::enable_shared_from_this<VRTexture> {
    private:
        ImageRecPtr img;
        int internal_format = -1;

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
        ImageRecPtr getImage();

        void read(string path);
        void write(string path);
};

OSG_END_NAMESPACE;

#endif // VRTEXTURE_H_INCLUDED
