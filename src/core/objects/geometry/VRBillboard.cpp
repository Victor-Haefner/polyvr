#include "VRBillboard.h"
#include "OSGGeometry.h"

#include <OpenSG/OSGGeoProperties.h>
#include <OpenSG/OSGGeoFunctions.h>
#include <OpenSG/OSGSimpleGeometry.h>        // Methods to create simple geos.
#include "core/tools/VRText.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRTexture.h"
#include "core/scene/VRSceneManager.h"

OSG_BEGIN_NAMESPACE;
using namespace std;


void VRBillboard::initBBMesh(bool alpha) {
    BBplane = makePlaneGeo(BBsizeW,BBsizeH,1,1);
    setMesh( OSGGeometry::create(BBplane) );

    BBmat = VRMaterial::create("billboard");
    setMaterial(BBmat);

    string wdir = VRSceneManager::get()->getOriginalWorkdir();
    BBmat->readVertexShader(wdir+"/shader/Billboard.vp");
    if (alpha) BBmat->enableTransparency();

    //BBtexChunk->setMinFilter(GL_NEAREST);
    //BBtexChunk->setMagFilter(GL_NEAREST);
    //BBtexChunk->setScale(false);

    //BBtexChunk->setInternalFormat(GL_RGBA_FLOAT32_ATI);

    //tex_env_chunk->setEnvMode(GL_NONE);

}

VRBillboardPtr VRBillboard::create(string name, bool alpha) { return shared_ptr<VRBillboard>(new VRBillboard(name, alpha) ); }
VRBillboardPtr VRBillboard::ptr() { return static_pointer_cast<VRBillboard>( shared_from_this() ); }

void VRBillboard::updateBBTexture() {
    BBtexChunk->setImage(BBtexture->getImage());
}

void VRBillboard::updateSize() {
    GeoPnt3fPropertyMTRecPtr pos = dynamic_cast<GeoPnt3fProperty *>(BBplane->getPositions());

    for (unsigned int i=0;i<pos->size();i++) {
        auto tmp = pos->getValue(i);
        tmp[0] *= BBsizeW*0.5/abs(tmp[0]);
        tmp[1] *= BBsizeH*0.5/abs(tmp[1]);
        pos->setValue(tmp, i);
    }
}

VRBillboard::VRBillboard(string name, bool alpha) : VRGeometry(name) {//TODO, BB Shader
    BBsizeH = BBsizeW = 1;
    initBBMesh(alpha);
}

VRBillboard::~VRBillboard() {}

void VRBillboard::setTexture(VRTexturePtr img) {
    BBtexture = img;
    updateBBTexture();
}

void VRBillboard::setSize(float sizeW, float sizeH) {
    BBsizeW = sizeW;
    BBsizeH = sizeH;
    updateSize();
}

void VRBillboard::createTestScene() {//Todo
    /*VRScene* scene = VRSceneManager::get()->makeDefaultScene("testBillboard");
    VRSceneManager::get()->VRSceneManager::get()->setKeyboardNavigationCentred(scene->getActiveCamera());
    scene->getActiveCamera()->setFrom(Vec3d(0,0,-1));
    scene->getActiveCamera()->setOrientation(Vec3d(0,0,0), Vec3d(0,1,0));
    VRBillboardPtr bb = new VRBillboard("bb");
    scene->add(bb);
    bb->setSize(1, 1);
    bb->setTexture(VRText::get()->create(" Hallo Hallo \n Hallo Hallo \n Hallo Hallo", "Mono.ttf", 200, 100, Color3f(0,0,0)));*/
}

OSG_END_NAMESPACE;
