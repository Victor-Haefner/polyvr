#include "VRGizmo.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/VRCamera.h"
#include "core/math/kinematics/VRConstraint.h"
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
        geo->setPickable(true);
        auto c = geo->getConstraint();
        c->lock({0,1,2,3,4,5});
        c->setLocal(true);
        c->setReferenceA( geo->getPose() );
        c->setReferential( ptr() );
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

    cRot->getConstraint()->free({3,4,5});
    cRotX->getConstraint()->free({4});
    cRotY->getConstraint()->free({4});
    cRotZ->getConstraint()->free({4});
    aTransX->getConstraint()->free({1});
    aTransY->getConstraint()->free({1});
    aTransZ->getConstraint()->free({1});
    aScaleX->getConstraint()->free({1});
    aScaleY->getConstraint()->free({1});
    aScaleZ->getConstraint()->free({1});
}

void VRGizmo::update() {
    if (!isVisible()) return;
    if (!target) return;

    auto bb = target->getWorldBoundingbox();
    setFrom( bb->center() );

    auto cam = VRScene::getCurrent()->getActiveCamera();
    auto cP = cam->getWorldPose();
    Vec3d cD  = cP->pos() - bb->center();

    auto P = Pose::create(Vec3d(), cP->x(), cD);
    P->makeDirOrthogonal();

    if (!cRot->isDragged()) cRot->setPose(P);
    else {
        auto wP = cRot->getWorldPose();
        wP->setUp( cD );
        wP->makeDirOrthogonal();
        cRot->setWorldPose(wP);
    }
    //cRot->getConstraint()->setReferenceA( cRot->getPose() );

    auto checkTransHandle = [&](VRTransformPtr t, int dof) {
        if (t->isDragged()) return;

        Vec3d f,u;
        f[dof] = 1.6;
        u[dof] = 1.0;
        if (cD[dof] < 0) {
            f[dof] *= -1;
            u[dof] *= -1;
        }

        Vec3d _u = t->getUp();
        if (_u[dof]*u[dof] > 0) return;

        t->setFrom(f);
        t->setUp(u);
        t->getConstraint()->setReferenceA(t->getPose());
    };

    auto checkScaleHandle = [&](VRTransformPtr t, int dof) {
        if (t->isDragged()) return;
        Vec3d f;
        f[dof] = 1.3;
        if (cD[dof] < 0) f[dof] *= -1;

        Vec3d _f = t->getFrom();
        if (_f[dof]*f[dof] > 0) return;
        t->setFrom(f);
    };

    checkTransHandle(aTransX, 0);
    checkTransHandle(aTransY, 1);
    checkTransHandle(aTransZ, 2);

    checkScaleHandle(aScaleX, 0);
    checkScaleHandle(aScaleY, 1);
    checkScaleHandle(aScaleZ, 2);


    auto processTranslate = [&](VRGeometryPtr t, int dof) {
        if (!mBase) {
            mBase = t->getWorldPose();
            tBase = target->getWorldPose();
        }

        auto tP = t->getWorldPose();
        double x = tP->pos()[dof] - mBase->pos()[dof];

        auto P = Pose::create( *tBase );
        Vec3d p;
        p[dof] = x;
        P->translate(p);
        target->setPose(P);
    };

    auto processScale = [&](VRGeometryPtr t, int dof) {
        if (!mBase) {
            mBase = t->getWorldPose();
            tBase = target->getWorldPose();
            sBase = target->getWorldScale();
        }

        auto tP = t->getWorldPose();
        double x = tP->pos()[dof] - mBase->pos()[dof];

        Vec3d s = sBase;
        s[dof] += x;
        target->setWorldScale(s);
    };

    auto processRotation = [&](VRGeometryPtr t, int dof) {
        if (!mBase) {
            mBase = t->getWorldPose();
            rBase1 = t->getEuler();
            tBase = target->getWorldPose();
            rBase2 = target->getEuler();
        }

        auto r = t->getEuler();
        double x = r[dof] - rBase1[dof];
        cout << " -- " << x << endl;

        auto P = Pose::create( *tBase );
        Vec3d s = rBase2;
        s[dof] += x;
        P->setEuler(s[0], s[1], s[2]);
        target->setWorldPose(P);
    };

    // check for dragging part
    if (aTransX->isDragged()) processTranslate(aTransX, 0);
    else if (aTransY->isDragged()) processTranslate(aTransY, 1);
    else if (aTransZ->isDragged()) processTranslate(aTransZ, 2);
    else if (aScaleX->isDragged()) processScale(aScaleX, 0);
    else if (aScaleY->isDragged()) processScale(aScaleY, 1);
    else if (aScaleZ->isDragged()) processScale(aScaleZ, 2);
    else if (cRotX->isDragged()) processRotation(cRotX, 0);
    else if (cRotY->isDragged()) processRotation(cRotY, 1);
    else if (cRotZ->isDragged()) processRotation(cRotZ, 2);
    else mBase = 0;
}



