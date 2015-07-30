#ifndef OSGSKYROOM_H_INCLUDED
#define OSGSKYROOM_H_INCLUDED

#include <OpenSG/OSGSkyBackground.h>
#include <OpenSG/OSGSimpleTexturedMaterial.h>
#include <OpenSG/OSGTextureBackground.h>
#include <OpenSG/OSGSolidBackground.h>
#include "core/utils/VRStorage.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRBackground : public VRStorage {
    public:
        enum TYPE { SOLID, IMAGE, SKY };

    private:
        BackgroundRecPtr bg;
        SkyBackgroundRecPtr sky;
        SolidBackgroundRecPtr sbg;
        TextureBackgroundRecPtr tbg;
        vector<ImageRecPtr> skyImgs;

        int type;
        string path;
        string format;
        Color3f color;

        TextureObjChunkRecPtr createSkyTexture();
        void updateSkyTextures();
        void initSky();

        void updateImgTexture();
        void initImg();

    protected:
        void update();

    public:
        VRBackground();

        void setBackground(TYPE t);
        BackgroundRecPtr getBackground();
        TYPE getBackgroundType();

        void setBackgroundColor(Color3f c);
        void setBackgroundPath(string s);
        Color3f getBackgroundColor();
        string getBackgroundPath();

        void setSkyBGExtension(string f);
        string getSkyBGExtension();

        void updateBackground();
};

OSG_END_NAMESPACE;

#endif // OSGSKYROOM_H_INCLUDED
