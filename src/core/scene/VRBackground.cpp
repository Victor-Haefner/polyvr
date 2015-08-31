#include "VRBackground.h"

#include <OpenSG/OSGSkyBackground.h>
#include <OpenSG/OSGSimpleTexturedMaterial.h>
#include <OpenSG/OSGImage.h>

#include "core/setup/VRSetupManager.h"
#include "core/setup/VRSetup.h"
#include "core/utils/toString.h"
#include "core/utils/VRStorage_template.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

TextureObjChunkRecPtr VRBackground::createSkyTexture() {
    ImageRecPtr image = Image::create();
    skyImgs.push_back(image);
    TextureObjChunkRecPtr chunk = TextureObjChunk::create();

    chunk->setImage(image);
    chunk->setMinFilter( GL_LINEAR_MIPMAP_LINEAR );
    chunk->setMagFilter( GL_LINEAR );
    chunk->setWrapS( GL_CLAMP_TO_EDGE );//GL_CLAMP //GL_REPEAT
    chunk->setWrapT( GL_CLAMP_TO_EDGE );//GL_CLAMP_TO_EDGE //GL_REPEAT
    //chunk->setEnvMode( GL_REPLACE );

    return chunk;
}

void VRBackground::updateSkyTextures() {
    string tmp;

    tmp = path + "_back" + format;
    skyImgs[0]->read(tmp.c_str());
    tmp = path + "_front" + format;
    skyImgs[1]->read(tmp.c_str());
    tmp = path + "_left" + format;
    skyImgs[2]->read(tmp.c_str());
    tmp = path + "_right" + format;
    skyImgs[3]->read(tmp.c_str());
    tmp = path + "_down" + format;
    skyImgs[4]->read(tmp.c_str());
    tmp = path + "_up" + format;
    skyImgs[5]->read(tmp.c_str());
}

void VRBackground::initSky() {
    sky = SkyBackground::create();
    sky->setBackTexture( createSkyTexture() );
    sky->setFrontTexture( createSkyTexture() );
    sky->setLeftTexture( createSkyTexture() );
    sky->setRightTexture( createSkyTexture() );
    sky->setBottomTexture( createSkyTexture() );
    sky->setTopTexture( createSkyTexture() );
}

void VRBackground::updateImgTexture() {
    skyImgs[6]->read(path.c_str());
}

void VRBackground::initImg() {
    tbg = TextureBackground::create();
    tbg->setTexture( createSkyTexture() );
}


VRBackground::VRBackground () {
    sbg = SolidBackground::create();
    initSky();
    initImg();

    color = Color3f(0.6, 0.6, 0.6);
    setBackground(SOLID);

    format = ".png";

    store("type", &type);
    store("color", &color);
    store("path", &path);
    store("format", &format);
}

void VRBackground::setBackground(TYPE t) {
    type = t;
    switch(t) {
        case SOLID:
            bg = sbg;
            sbg->setColor(color);
            break;
        case IMAGE:
            bg = tbg;
            updateImgTexture();
            break;
        case SKY:
            bg = sky;
            updateSkyTextures();
            break;
    }

    updateBackground();
}

void VRBackground::setBackgroundColor(Color3f c) {
    color = c;
    sbg->setColor(c);
    setBackground(TYPE(type));
}

void VRBackground::setBackgroundPath(string s) {
    path = s;
    if (type == IMAGE) updateImgTexture();
    if (type == SKY) updateSkyTextures();
    setBackground(TYPE(type));
}

BackgroundRecPtr VRBackground::getBackground() { return bg; }
void VRBackground::setSkyBGExtension(string f) { format = f; updateSkyTextures(); }
string VRBackground::getSkyBGExtension() { return format; }
VRBackground::TYPE VRBackground::getBackgroundType() { return TYPE(type); }
Color3f VRBackground::getBackgroundColor() { return color; }
string VRBackground::getBackgroundPath() { return path; }

void VRBackground::updateBackground() {
    VRSetupManager::getCurrent()->setViewBackground(getBackground());
}

void VRBackground::update() { setBackground(TYPE(type)); }


OSG_END_NAMESPACE;
