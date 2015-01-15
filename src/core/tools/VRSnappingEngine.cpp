#include "VRSnappingEngine.h"
#include "core/objects/VRTransform.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/setup/devices/VRDevice.h"
#include "core/setup/VRSetup.h"
#include "core/setup/VRSetupManager.h"
#include "core/scene/VRScene.h"
#include "core/scene/VRSceneManager.h"
#include "addons/Engineering/CSG/Octree/Octree.h"

OSG_BEGIN_NAMESPACE;

VRSnappingEngine::VRSnappingEngine() {
    hintGeo = new VRGeometry("snapping_engine_hint");
    positions = new Octree(0.1);
    distances = new Octree(0.1);
    lines = new Octree(0.1);
    planes = new Octree(0.1);
    orientations = new Octree(0.1);

    VRFunction<int>* fkt = new VRFunction<int>("snapping engine update", boost::bind(&VRSnappingEngine::update, this) );
    VRSceneManager::getCurrent()->addUpdateFkt(fkt, 100);
}

void VRSnappingEngine::clear() {
    //hintGeo->clear();
    //hintGeo->hide();
    positions->clear();
    distances->clear();
    lines->clear();
    planes->clear();
    orientations->clear();
}

void VRSnappingEngine::setPreset(PRESET preset) {
    clear();
    switch(preset) {
        case SIMPLE_ALIGNMENT:
            // TODO
            addDistance(1);
            setOrientation(true);
            addAxis( Line(Pnt3f(0,0,0), Vec3f(1,0,0)) );
            addAxis( Line(Pnt3f(0,0,0), Vec3f(0,1,0)) );
            addAxis( Line(Pnt3f(0,0,0), Vec3f(0,0,1)) );
            break;
    }
}

void VRSnappingEngine::addObject(VRTransform* obj, float weight) {
    objects[obj] = obj->getWorldMatrix();
    Vec3f p = obj->getWorldPosition();
    positions->add(p[0], p[1], p[2], obj);
}

// snap object's position
void VRSnappingEngine::addDistance(float dist, bool local, float weight) {
    distances->add(dist, 0, 0, new float(dist) );
}

void VRSnappingEngine::addAxis(Line line, bool local, float weight) {
    Vec3f d = line.getDirection();
    lines->add(d[0], d[1], d[2], new Vec3f(d));
    lines->add(-d[0], -d[1], -d[2], new Vec3f(-d));
}

void VRSnappingEngine::addPlane(Plane plane, bool local, float weight) {}

// snap object's orientation
void VRSnappingEngine::setOrientation(bool b, bool local, float weight) {
    doOrientation = b;
}

void VRSnappingEngine::setVisualHints(bool b) {
    showHints = b;
    hintGeo->setVisible(b);
}

void VRSnappingEngine::update() {
    // get dragged objects
    for (auto dev : VRSetupManager::getCurrent()->getDevices()) {
        VRTransform* obj = dev.second->getDraggedObject();
        VRTransform* gobj = dev.second->getDraggedGhost();
        if (obj == 0) continue;

        //Matrix m = obj->getWorldMatrix();
        Matrix m = gobj->getWorldMatrix();
        Vec3f p = Vec3f(m[3]);

        vector<void*> neighbors = positions->radiusSearch(p[0], p[1], p[2], influence_radius);
        for (auto n : neighbors) {
            VRTransform* t = (VRTransform*)n;
            if (t == obj) continue;
            Matrix nm = t->getWorldMatrix();
            Vec3f np = Vec3f(nm[3]);

            Vec3f dir = p-np;
            float d = dir.length();
            if (d < 1e-4) continue;
            dir /= d;

            if (doOrientation) {
                m[0] = nm[0];
                m[1] = nm[1];
                m[2] = nm[2];
            }

            // get snapping axis
            vector<void*> lins = lines->radiusSearch(dir[0],dir[1],dir[2], distance_snap);
            if (lins.size() != 0) { // apply snap
                dir = *(Vec3f*)lins[0];
                Line l(np, dir);
                m.setTranslate(l.getClosestPoint(p));
            }

            // get snap distances in snapping range
            vector<void*> dists = distances->radiusSearch(d,0,0, distance_snap*d);
            if (dists.size() != 0) { // apply snap
                d = *(float*)dists[0];
                m.setTranslate(np + dir*d);
            }
        }

        obj->setWorldMatrix(m);
    }

    // update geo
    if (!hintGeo->isVisible()) return;
}

OSG_END_NAMESPACE;
