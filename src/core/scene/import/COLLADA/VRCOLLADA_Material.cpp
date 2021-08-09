#include "VRCOLLADA_Material.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRTexture.h"

using namespace OSG;

VRCOLLADA_Material::VRCOLLADA_Material() {}
VRCOLLADA_Material::~VRCOLLADA_Material() {}

VRCOLLADA_MaterialPtr VRCOLLADA_Material::create() { return VRCOLLADA_MaterialPtr( new VRCOLLADA_Material() ); }
VRCOLLADA_MaterialPtr VRCOLLADA_Material::ptr() { return static_pointer_cast<VRCOLLADA_Material>(shared_from_this()); }

void VRCOLLADA_Material::setupOntology(VROntologyPtr o) {
    ontology = o;
}

void VRCOLLADA_Material::finalize() {
    for (auto m : mappings) {
        //cout << "VRCOLLADA_Material::finalize " << m.first << " <- " << m.second << endl;
        auto effect = library_effects[m.second];
        auto material = library_materials[m.first];
        material->setDiffuse( effect->getDiffuse() );
        material->setSpecular( effect->getSpecular() );
        material->setAmbient( effect->getAmbient() );
        material->setEmission( effect->getEmission() );
        material->setShininess( effect->getShininess() );
        material->setTexture( effect->getTexture() );
    }
}

void VRCOLLADA_Material::addSurface(string id) { currentSurface = id; /*cout << "VRCOLLADA_Material::addSurface " << id << endl;*/ }
void VRCOLLADA_Material::addSampler(string id) { currentSampler = id; /*cout << "VRCOLLADA_Material::addSampler " << id << endl;*/ }

void VRCOLLADA_Material::setSurfaceSource(string source) { surface[currentSurface] = source; /*cout << "VRCOLLADA_Material::setSurfaceSource " << currentSurface << " " << source << endl;*/ }
void VRCOLLADA_Material::setSamplerSource(string source) { sampler[currentSampler] = source; /*cout << "VRCOLLADA_Material::setSamplerSource " << currentSampler << " " << source << endl;*/ }
void VRCOLLADA_Material::setFilePath(string fPath) { filePath = fPath; }

void VRCOLLADA_Material::loadImage(string id, string path) {
    auto img = VRTexture::create();
    img->read(filePath+"/"+path);
    library_images[id] = img;
    //cout << "VRCOLLADA_Material::loadImage " << id << ", " << filePath+"/"+path << endl;
}

void VRCOLLADA_Material::setTexture(string source) {
    auto samplerSrc = sampler[source];
    auto surfaceSrc = surface[samplerSrc];
    auto img = library_images[surfaceSrc];
    currentEffect->setTexture(img);
    //cout << "VRCOLLADA_Material::setTexture " << source << ", " << samplerSrc << ", " << surfaceSrc << endl;
}

void VRCOLLADA_Material::newEffect(string id) {
    auto m = VRMaterial::create();
    library_effects[id] = m;
    currentEffect = m;
}

void VRCOLLADA_Material::newMaterial(string id, string name) {
    auto m = VRMaterial::create(name);
    library_materials[id] = m;
    currentMaterial = id;
}

void VRCOLLADA_Material::closeEffect() {
    currentEffect = 0;
}

void VRCOLLADA_Material::closeMaterial() {
    currentMaterial = "";
}

void VRCOLLADA_Material::setMaterialEffect(string eid) {
    mappings[currentMaterial] = eid;
}

void VRCOLLADA_Material::setColor(string sid, Color4f col) {
    //cout << " -- VRCOLLADA_Material::setColor " << sid << "  " << col << endl;
    if (!currentEffect) return;
    Color3f c3 = Color3f(col[0], col[1], col[2]);
    if (sid == "diffuse") currentEffect->setDiffuse(c3);
    if (sid == "specular") currentEffect->setSpecular(c3);
    if (sid == "ambient") currentEffect->setAmbient(c3);
    if (sid == "emission") currentEffect->setEmission(c3);
}

void VRCOLLADA_Material::setShininess(float f) {
    if (!currentEffect) return;
    currentEffect->setShininess(f*0.01);
}

VRMaterialPtr VRCOLLADA_Material::getMaterial(string sid) { return library_materials[sid]; }
