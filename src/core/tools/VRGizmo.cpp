#include "VRGizmo.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/VRCamera.h"
#include "core/math/kinematics/VRConstraint.h"
#include "core/math/partitioning/boundingbox.h"
#include "core/math/pose.h"
#include "core/utils/VRFunction.h"
#include "core/scene/VRScene.h"

#define GLSL(shader) #shader

using namespace OSG;

VRGizmo::VRGizmo(string name) : VRTransform(name) {
    updateCb = VRUpdateCb::create( "player", bind(&VRGizmo::update, this));
    VRScene::getCurrent()->addUpdateFkt(updateCb);
}

VRGizmo::~VRGizmo() {}

VRGizmoPtr VRGizmo::create(string name) { auto g = VRGizmoPtr( new VRGizmo(name) ); g->setup(); return g; }
VRGizmoPtr VRGizmo::ptr() { return static_pointer_cast<VRGizmo>(shared_from_this()); }

void VRGizmo::setTarget(VRTransformPtr t) {
    target = t;
    auto bb = target->getBoundingbox();
    tOffset = bb->center();
}

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

    auto mRotation = setupMaterial("rotation", Color3f(1,0,0));
    mRotation->setVertexShader(rotVP, "rotVP");
    mRotation->setFragmentShader(rotFP, "rotFP");

    cRot    = VRGeometry::create("cRot", "Annulus", "1.0 0.9 16");
    cRotX   = VRGeometry::create("cRotX", "Annulus", "1.0 0.9 16");
    cRotY   = VRGeometry::create("cRotY", "Annulus", "1.0 0.9 16");
    cRotZ   = VRGeometry::create("cRotZ", "Annulus", "1.0 0.9 16");
    aTransX = VRGeometry::create("aTransX", "Cone", "0.2 0.05 16");
    aTransY = VRGeometry::create("aTransY", "Cone", "0.2 0.05 16");
    aTransZ = VRGeometry::create("aTransZ", "Cone", "0.2 0.05 16");
    aScaleX = VRGeometry::create("aScaleX", "Cylinder", "0.2 0.05 16");
    aScaleY = VRGeometry::create("aScaleY", "Cylinder", "0.2 0.05 16");
    aScaleZ = VRGeometry::create("aScaleZ", "Cylinder", "0.2 0.05 16");

    auto setVertCols = [](VRGeometryPtr g, Color3f c) {
        VRGeoData data( g );
        data.addVertexColors( c );
    };

    setVertCols(cRot,  Color3f(0.9,0.9,0.9));
    setVertCols(cRotX, Color3f(1,0,0));
    setVertCols(cRotY, Color3f(0,1,0));
    setVertCols(cRotZ, Color3f(0,0,1));

    setupPart(cRot,    mRotation, Vec3d(0,0,0),   Vec3d(1,1,1),  Vec3d(0,1,0));
    setupPart(cRotX,   mRotation, Vec3d(0,0,0),   Vec3d(0,1,-1), Vec3d(1,0,0));
    setupPart(cRotY,   mRotation, Vec3d(0,0,0),   Vec3d(1,0,-1), Vec3d(0,1,0));
    setupPart(cRotZ,   mRotation, Vec3d(0,0,0),   Vec3d(1,-1,0), Vec3d(0,0,1));
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

    auto cam = VRScene::getCurrent()->getActiveCamera();
    auto cP = cam->getWorldPose();

    Pose T ( tOffset);
    Pose tP = (*target->getWorldPose()) * T;
    Vec3d cD  = cP->pos() - tP.pos();

    bool anyRotDragged = cRot->isDragged() || cRotX->isDragged() || cRotY->isDragged() || cRotZ->isDragged();
    bool anyTransDragged = aTransX->isDragged() || aTransY->isDragged() || aTransZ->isDragged();
    bool anyScaleDragged = aScaleX->isDragged() || aScaleY->isDragged() || aScaleZ->isDragged();
    bool anyDragged = anyRotDragged || anyTransDragged || anyScaleDragged;

    if (!(anyRotDragged || anyScaleDragged)) setFrom(tP.pos());

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

        Vec3d _f = t->getFrom();
        if (abs(_f[dof]-f[dof]) < 1e-3) return;

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
        if (abs(_f[dof]-f[dof]) < 1e-3) return;
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

        Pose T ( tOffset);
        Pose Ti(-tOffset);

        Pose B = (*tBase);
        Pose Br(Vec3d(), B.dir(), B.up());
        Pose Bt( B.pos() );

        Pose S;
        Vec3d s = Vec3d(1,1,1);
        s[dof] += x;
        S.setScale( s );

        Pose Bs;
        Bs.setScale( B.scale() );

        target->setWorldPose( Pose::create( Bt * S * Br * Bs ) );
    };

    auto processRotation = [&](VRGeometryPtr t, int dof) {
        if (!mBase) {
            mBase = target->getWorldPose();
            rBase = t->getEuler();
        }

        auto r = t->getEuler();
        double x = (r[dof] - rBase[dof])*8.0;

        Pose T ( tOffset);
        Pose Ti(-tOffset);

        Pose R;
        Vec3d D; D[dof] = 1;
        R.rotate(x, D);

        Pose B = (*mBase) * T;
        Pose Br(Vec3d(), B.dir(), B.up());
        Pose Bt( B.pos() );

        Pose S;
        S.setScale( B.scale() );

        target->setWorldPose( Pose::create( Bt * R * Br * S * Ti ) );
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

string VRGizmo::rotVP =
"#version 120\n"
GLSL(
attribute vec4 osg_Vertex;
attribute vec4 osg_Color;

varying vec4 vertPos;
varying vec4 color;

void main(void) {
 	vertPos = osg_Vertex;
 	color = osg_Color;
 	gl_Position = gl_ModelViewProjectionMatrix*osg_Vertex;
}
);

string VRGizmo::rotFP =
"#version 120\n"
GLSL(
varying vec4 vertPos;
varying vec4 color;

void main(void) {
 	vec3 pos = vertPos.xyz;
 	bool isArrow = bool(abs(pos.x) > 0.802 && abs(pos.z) < 0.5);

 	float r1 = 0.9;
 	float r2 = 1.0;
 	float rm = (r1+r2)*0.5;
 	float w = r2-r1;
 	float l = length(pos);
 	float r = abs(l-rm)/w;

 	if (isArrow) {
 		if (r > 0.25) discard;
 		gl_FragColor = color;
 	} else {
 		if (r > 0.1) discard;
 		gl_FragColor = vec4(0.8,0.8,0.8,1.0);
 	}
}
);

