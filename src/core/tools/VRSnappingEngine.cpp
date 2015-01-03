#include "VRSnappingEngine.h"
#include "core/objects/VRTransform.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/setup/devices/VRDevice.h"
#include "core/setup/VRSetup.h"
#include "core/setup/VRSetupManager.h"
#include "addons/Engineering/CSG/Octree/Octree.h"

OSG_BEGIN_NAMESPACE;

VRSnappingEngine::VRSnappingEngine() {
    hintGeo = new VRGeometry("snapping_engine_hint");
    positions = new Octree(0.1);
    distances = new Octree(0.1);
    orientations = new Octree(0.1);
}

void VRSnappingEngine::clear() {
    //hintGeo->clear();
    //hintGeo->hide();
    positions->clear();
    distances->clear();
    orientations->clear();
}

void VRSnappingEngine::setPreset(PRESET preset) {
    clear();
    switch(preset) {
        case SIMPLE_ALIGNMENT:
            // TODO
            addDistance(1);
            break;
    }
}

void VRSnappingEngine::addObject(VRTransform* obj, float weight) {
    objects[obj] = obj->getWorldMatrix();
}

// snap object's position
void VRSnappingEngine::addDistance(float dist, bool local, float weight) {
    distances->add(dist, 0, 0, new float(dist) );
}

void VRSnappingEngine::addAxis(Line line, bool local, float weight) {}
void VRSnappingEngine::addPlane(Plane plane, bool local, float weight) {}

// snap object's orientation
void VRSnappingEngine::addDirection(Vec3f dir, bool local, float weight) {}
void VRSnappingEngine::addUp(Vec3f up, bool local, float weight) {}

void VRSnappingEngine::setVisualHints(bool b) {
    showHints = b;
    hintGeo->setVisible(b);
}

void VRSnappingEngine::update() {
    // get dragged objects
    vector<VRTransform*> dragged;
    for (auto dev : VRSetupManager::getCurrent()->getDevices()) {
        VRTransform* obj = dev.second->getDraggedObject();
        if (obj) dragged.push_back(obj);
    }

    // update rules
    if (dragged.size() == 0) return;
    for (auto obj : dragged) {
        Matrix m = obj->getWorldMatrix();
        Vec3f p = Vec3f(m[3]);

        vector<void*> neighbors = positions->radiusSearch(p[0], p[1], p[2], influence_radius);
        for (auto n : neighbors) {
            VRTransform* t = (VRTransform*)n;
            Matrix nm = t->getWorldMatrix();
            Vec3f np = Vec3f(nm[3]);
            float d2 = (p-np).squareLength();

            // get snap distances in snapping range
            vector<void*> dists = distances->radiusSearch(d2,0,0,distance_snap);
            if (dists.size() == 0) continue;

            // apply snap
            float d = sqrt(*(float*)dists[0]);
            p = np + (p-np)*d/sqrt(d2);
        }
    }

    // update geo
    if (!hintGeo->isVisible()) return;
}

OSG_END_NAMESPACE;
