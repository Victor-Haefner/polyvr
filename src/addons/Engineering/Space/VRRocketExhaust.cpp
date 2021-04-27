#include "VRRocketExhaust.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/material/VRMaterial.h"
#include "core/scene/VRScene.h"

using namespace OSG;

VRRocketExhaust::VRRocketExhaust(string name) : VRGeometry(name) { init(); }
VRRocketExhaust::~VRRocketExhaust() {}

VRRocketExhaustPtr VRRocketExhaust::create(string name) { return VRRocketExhaustPtr( new VRRocketExhaust(name) ); }
VRRocketExhaustPtr VRRocketExhaust::ptr() { return static_pointer_cast<VRRocketExhaust>(shared_from_this()); }

void VRRocketExhaust::init() {
    mat = VRMaterial::create("exhaust");
    mat->setLit(false);
    mat->setTransparency(0.3);
    mat->setFrontBackModes(GL_NONE, GL_FILL);
    mat->addPass();
    mat->setLit(false);
    mat->setTransparency(0.3);
    mat->setFrontBackModes(GL_FILL, GL_NONE);

    updateCb = VRUpdateCb::create( "exhaustEffects", bind(&VRRocketExhaust::update, this) );
    VRScene::getCurrent()->addUpdateFkt(updateCb);
}

void VRRocketExhaust::update() {
    if (amount < 1e-3) return;

    cycle += 1;
    if (cycle >= 100) cycle = 0;

    float n = 0.95 + 0.05 * rand()/float(RAND_MAX);
    float s = cos(cycle*0.01*2*Pi)*0.03+0.97 * n;
    setScale(Vec3d(1,1,s));
}

void VRRocketExhaust::set(float amount) {
    this->amount = amount;

    int Nz = 64;
    int Ns = 16;
    float Lmin = 1;
    float Lmax = 30;
    float L = Lmin+amount*(30-Lmin);

    VRGeoData data;
    for (int i=0; i<Nz; i++) {
        float l = float(i)/(Nz-1);
        float h = l*L;
        float f = max(abs(cos(h)), abs(cos(h+float(Pi*0.5))));
        float r = f * (1-l*l) * amount;
        float I = (1-f*0.7)*0.6*(0.5+amount*0.5)*(1-l*l);
        for (int j=0; j<Ns; j++) {
            float a = float(j)/(Ns-1)*2*Pi;
            Vec3d n = Vec3d(cos(a), sin(a), 0);
            Vec3d p = Vec3d(r*cos(a), r*sin(a), -h);
            Color4f c = Color4f(0.9,1,1,I);
            data.pushVert(p, n, c);
        }

        if (i == 0) continue;

        for (int j=1; j<Ns; j++) {
            data.pushQuad(j+Ns*(i-1), j-1+Ns*(i-1), j-1+Ns*i, j+Ns*i);
        }
    }
    data.apply(ptr());

    setMaterial(mat);
}
