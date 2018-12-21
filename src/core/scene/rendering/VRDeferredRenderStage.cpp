#include "VRDeferredRenderStage.h"
#include "core/objects/object/VRObject.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/material/VRMaterial.h"
#include "VRDefShading.h"

using namespace OSG;

VRDeferredRenderStage::VRDeferredRenderStage(string name) {
    root = VRObject::create(name+"_DRS_bottom");
    mat = setupRenderLayer(name);
    layer->addChild(root);
}

VRDeferredRenderStage::~VRDeferredRenderStage() {}

VRDeferredRenderStagePtr VRDeferredRenderStage::create(string name) { return VRDeferredRenderStagePtr( new VRDeferredRenderStage(name) ); }

VRMaterialPtr VRDeferredRenderStage::setupRenderLayer(string name) {
    layer = VRGeometry::create(name+"_renderlayer");
    auto mat = VRMaterial::create(name+"_mat");
    string s = "2"; // TODO: check if layers are not culled in CAVE!
    layer->setPrimitive("Plane "+s+" "+s+" 1 1");
    layer->setMaterial( mat );
    layer->setMeshVisibility(false);
    //layer->hide("SHADOW");
    mat->setDepthTest(GL_ALWAYS);
    //mat->setSortKey(1000);
    return mat;
}

void VRDeferredRenderStage::initDeferred() {
    defRendering = shared_ptr<VRDefShading>( new VRDefShading() );
    defRendering->initDeferredShading(root);
    defRendering->setDeferredShading(false);
}

void VRDeferredRenderStage::setCamera(OSGCameraPtr cam) {
    if (auto r = getRendering()) r->setDSCamera(cam);
}

void VRDeferredRenderStage::addLight(VRLightPtr l) {
    if (auto r = getRendering()) r->addDSLight(l);
}

VRObjectPtr VRDeferredRenderStage::getTop() { return layer; }
VRObjectPtr VRDeferredRenderStage::getBottom() { return root; }
VRMaterialPtr VRDeferredRenderStage::getMaterial() { return mat; }
VRGeometryPtr VRDeferredRenderStage::getLayer() { return layer; }
shared_ptr<VRDefShading> VRDeferredRenderStage::getRendering() { return defRendering; }
//shared_ptr<VRDefShading> VRDeferredRenderStage::getRendering() { if (!defRendering) initDeferred(); return defRendering; }

void VRDeferredRenderStage::setActive(bool da, bool la) {
    if (defRendering) defRendering->setDeferredShading(da);
    layer->setMeshVisibility(la);
}

void VRDeferredRenderStage::insert(shared_ptr<VRDeferredRenderStage> stage) {
    stage->getTop()->switchParent( getBottom() );
    if (child) child->getTop()->switchParent( stage->getBottom() );
    //cout << "VRDeferredRenderStage::insert " << stage.get() << "  bottom " << getBottom()->getName() << endl;
    /*for (auto l : getBottom()->getLinks()) { // TODO: is this needed??
        cout << " move link " << l->getName() << " to " << stage->getBottom()->getName() << endl;
        stage->getBottom()->addLink(l);
        getBottom()->remLink(l);
    }*/
    child = stage;
}


