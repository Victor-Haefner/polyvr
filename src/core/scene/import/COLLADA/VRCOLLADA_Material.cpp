#include "VRCOLLADA_Material.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRTexture.h"
#include "core/utils/VRScheduler.h"

using namespace OSG;

VRCOLLADA_Material::VRCOLLADA_Material() { scheduler = VRScheduler::create(); }
VRCOLLADA_Material::~VRCOLLADA_Material() {}

VRCOLLADA_MaterialPtr VRCOLLADA_Material::create() { return VRCOLLADA_MaterialPtr( new VRCOLLADA_Material() ); }
VRCOLLADA_MaterialPtr VRCOLLADA_Material::ptr() { return static_pointer_cast<VRCOLLADA_Material>(shared_from_this()); }

void VRCOLLADA_Material::finalize() {
    scheduler->callPostponed(true);
}

void VRCOLLADA_Material::addSurface(string id) { currentSurface = id; }
void VRCOLLADA_Material::addSampler(string id) { currentSampler = id; }

void VRCOLLADA_Material::setSurfaceSource(string source) { surface[currentSurface] = source; }
void VRCOLLADA_Material::setSamplerSource(string source) { sampler[currentSampler] = source; }

void VRCOLLADA_Material::setFilePath(string fPath) { filePath = fPath; }

void VRCOLLADA_Material::loadImage(string id, string path) {
    auto img = VRTexture::create();
    img->read(filePath+"/"+path);
    library_images[id] = img;
    //cout << "VRCOLLADA_Material::loadImage " << id << ", " << filePath+"/"+path << endl;
}

void VRCOLLADA_Material::setTexture(string source, string eid) {
    if (eid == "") eid = currentEffect;
    if (!sampler.count(source)) {
        scheduler->postpone( bind(&VRCOLLADA_Material::setTexture, this, source, eid) );
        return;
    }

    auto samplerSrc = sampler[source];

    if (!surface.count(samplerSrc)) {
        scheduler->postpone( bind(&VRCOLLADA_Material::setTexture, this, source, eid) );
        return;
    }

    auto surfaceSrc = surface[samplerSrc];

    if (!library_images.count(surfaceSrc) || !library_effects.count(eid)) {
        scheduler->postpone( bind(&VRCOLLADA_Material::setTexture, this, source, eid) );
        return;
    }

    auto img = library_images[surfaceSrc];
    library_effects[eid]->setTexture(img);
}

void VRCOLLADA_Material::newEffect(string id) {
    auto m = VRMaterial::create();
    library_effects[id] = m;
    currentEffect = id;
}

void VRCOLLADA_Material::newMaterial(string id, string name) {
    auto m = VRMaterial::create(name);
    library_materials[id] = m;
    currentMaterial = id;
}

void VRCOLLADA_Material::closeEffect() {
    currentEffect = "";
}

void VRCOLLADA_Material::closeMaterial() {
    currentMaterial = "";
}

void VRCOLLADA_Material::setMaterialEffect(string eid, string mid) {
    if (mid == "") mid = currentMaterial;
    if (!library_effects.count(eid) || !library_materials.count(mid)) {
        scheduler->postpone( bind(&VRCOLLADA_Material::setMaterialEffect, this, eid, mid) );
        return;
    }

    auto effect = library_effects[eid];
    auto material = library_materials[mid];
    material->setDiffuse( effect->getDiffuse() );
    material->setSpecular( effect->getSpecular() );
    material->setAmbient( effect->getAmbient() );
    material->setEmission( effect->getEmission() );
    material->setShininess( effect->getShininess() );
    material->setTexture( effect->getTexture() );
    material->setLit( effect->isLit() );
}

void VRCOLLADA_Material::setRendering(string method, string eid) {
    if (eid == "") eid = currentEffect;
    if (!library_effects.count(eid)) {
        scheduler->postpone( bind(&VRCOLLADA_Material::setRendering, this, method, eid) );
        return;
    }

    auto effect = library_effects[eid];
    if (method == "constant") effect->setLit(0);
    if (method == "lambert") effect->setLit(1);
    if (method == "phong") effect->setLit(1);
}

void VRCOLLADA_Material::setColor(string sid, Color4f col, string eid) {
    if (eid == "") eid = currentEffect;
    if (!library_effects.count(eid)) {
        scheduler->postpone( bind(&VRCOLLADA_Material::setColor, this, sid, col, eid) );
        return;
    }

    auto effect = library_effects[eid];
    Color3f c3 = Color3f(col[0], col[1], col[2]);
    if (sid == "diffuse") effect->setDiffuse(c3);
    if (sid == "specular") effect->setSpecular(c3);
    if (sid == "ambient") effect->setAmbient(c3);
    if (sid == "emission") effect->setEmission(c3);
}

void VRCOLLADA_Material::setShininess(float f, string eid) {
    if (eid == "") eid = currentEffect;
    if (!library_effects.count(eid)) {
        scheduler->postpone( bind(&VRCOLLADA_Material::setShininess, this, f, eid) );
        return;
    }
    library_effects[eid]->setShininess(f*0.01);
}

VRMaterialPtr VRCOLLADA_Material::getMaterial(string sid) {
    if (!library_materials.count(sid)) return 0;
    return library_materials[sid];
}
