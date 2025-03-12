#include "VRGizmo.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/VRCamera.h"
#include "core/math/partitioning/boundingbox.h"
#include "core/math/pose.h"
#include "core/utils/VRFunction.h"
#include "core/scene/VRScene.h"

using namespace OSG;

VRGizmo::VRGizmo(string name) : VRTransform(name) {
    updateCb = VRUpdateCb::create( "player", bind(&VRGizmo::update, this));
    VRScene::getCurrent()->addUpdateFkt(updateCb);
}

VRGizmo::~VRGizmo() {}

VRGizmoPtr VRGizmo::create(string name) { auto g = VRGizmoPtr( new VRGizmo(name) ); g->setup(); return g; }
VRGizmoPtr VRGizmo::ptr() { return static_pointer_cast<VRGizmo>(shared_from_this()); }

void VRGizmo::setTarget(VRTransformPtr t) { target = t; }

void VRGizmo::setup() {
    auto setupMaterial = [&](string name, Color3f c) {
        auto m = VRMaterial::create(name);
        m->setLit(false);
        m->setDiffuse(c);
        return m;
    };

    auto setupPart = [&](VRGeometryPtr geo, VRMaterialPtr mat, Vec3d p, Vec3d d, Vec3d u) {
        addChild(geo);
        geo->setMaterial(mat);
        geo->setTransform(p,d,u);
    };

    auto mWhite = setupMaterial("white", Color3f(0.9,0.9,0.9));
    auto mGray = setupMaterial("gray", Color3f(0.4,0.4,0.4));
    auto mRed = setupMaterial("red", Color3f(1,0,0));
    auto mGreen = setupMaterial("green", Color3f(0,1,0));
    auto mBlue = setupMaterial("blue", Color3f(0,0,1));

    cRot    = VRGeometry::create("cRot", "Annulus", "1.1 0.95 16");
    cRotX   = VRGeometry::create("cRotX", "Annulus", "1.1 0.95 16");
    cRotY   = VRGeometry::create("cRotY", "Annulus", "1.1 0.95 16");
    cRotZ   = VRGeometry::create("cRotZ", "Annulus", "1.1 0.95 16");
    aTransX = VRGeometry::create("aTransX", "Cone", "0.2 0.075 16");
    aTransY = VRGeometry::create("aTransY", "Cone", "0.2 0.075 16");
    aTransZ = VRGeometry::create("aTransZ", "Cone", "0.2 0.075 16");
    aScaleX = VRGeometry::create("aScaleX", "Cylinder", "0.2 0.075 16");
    aScaleY = VRGeometry::create("aScaleY", "Cylinder", "0.2 0.075 16");
    aScaleZ = VRGeometry::create("aScaleZ", "Cylinder", "0.2 0.075 16");

    setupPart(cRot,    mWhite, Vec3d(0,0,0),   Vec3d(1,1,1),  Vec3d(0,1,0));
    setupPart(cRotX,   mRed,   Vec3d(0,0,0),   Vec3d(0,0,-1), Vec3d(1,0,0));
    setupPart(cRotY,   mGreen, Vec3d(0,0,0),   Vec3d(0,0,-1), Vec3d(0,1,0));
    setupPart(cRotZ,   mBlue,  Vec3d(0,0,0),   Vec3d(0,-1,0), Vec3d(0,0,1));
    setupPart(aScaleX, mRed,   Vec3d(1.3,0,0), Vec3d(0,0,-1), Vec3d(1,0,0));
    setupPart(aScaleY, mGreen, Vec3d(0,1.3,0), Vec3d(0,0,-1), Vec3d(0,1,0));
    setupPart(aScaleZ, mBlue,  Vec3d(0,0,1.3), Vec3d(0,-1,0), Vec3d(0,0,1));
    setupPart(aTransX, mRed,   Vec3d(1.6,0,0), Vec3d(0,0,-1), Vec3d(1,0,0));
    setupPart(aTransY, mGreen, Vec3d(0,1.6,0), Vec3d(0,0,-1), Vec3d(0,1,0));
    setupPart(aTransZ, mBlue,  Vec3d(0,0,1.6), Vec3d(0,-1,0), Vec3d(0,0,1));
}

void VRGizmo::update() {
    if (!isVisible()) return;
    if (!target) return;

    auto bb = target->getWorldBoundingbox();
    setFrom( bb->center() );

    auto cam = VRScene::getCurrent()->getActiveCamera();
    auto cP = cam->getWorldPose();
    Vec3d d  = cP->pos() - bb->center();

    auto P = Pose::create(Vec3d(), cP->x(), d);
    P->makeDirOrthogonal();

    cRot->setPose(P);

    if (d[0] < 0) {
        aScaleX->setFrom( Vec3d(-1.3,0,0) );
        aTransX->setFrom( Vec3d(-1.6,0,0) );
        aTransX->setUp(   Vec3d(-1,0,0)   );
    } else {
        aScaleX->setFrom( Vec3d( 1.3,0,0) );
        aTransX->setFrom( Vec3d( 1.6,0,0) );
        aTransX->setUp(   Vec3d( 1,0,0)   );
    }

    if (d[1] < 0) {
        aScaleY->setFrom( Vec3d(0,-1.3,0) );
        aTransY->setFrom( Vec3d(0,-1.6,0) );
        aTransY->setUp(   Vec3d(0,-1,0)   );
    } else {
        aScaleY->setFrom( Vec3d( 0,1.3,0) );
        aTransY->setFrom( Vec3d( 0,1.6,0) );
        aTransY->setUp(   Vec3d( 0,1,0)   );
    }

    if (d[2] < 0) {
        aScaleZ->setFrom( Vec3d(0,0,-1.3) );
        aTransZ->setFrom( Vec3d(0,0,-1.6) );
        aTransZ->setUp(   Vec3d(0,0,-1)   );
    } else {
        aScaleZ->setFrom( Vec3d(0,0,1.3) );
        aTransZ->setFrom( Vec3d(0,0,1.6) );
        aTransZ->setUp(   Vec3d(0,0,1)   );
    }
}



