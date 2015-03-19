#include "VRAMLLoader.h"

#include <iostream>
//#include <collada/dom.h>
#include <collada/dae.h>
#include <OpenSG/OSGSceneFileHandler.h>
#include <OpenSG/OSGGeoProperties.h>
#include <OpenSG/OSGGeometry.h>


#include "core/objects/object/VRObject.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/material/VRMaterial.h"

OSG_BEGIN_NAMESPACE;

VRAMLLoader::VRAMLLoader() {
    ;
}

VRAMLLoader* VRAMLLoader::get() {
    static VRAMLLoader* l = new VRAMLLoader();
    return l;
}

struct Geo {
    int Np = 0;
    int Nn = 0;
    GeoVectorPropertyRecPtr pos = 0;
    GeoVectorPropertyRecPtr norms = 0;
    GeoIntegralPropertyRefPtr inds_p = 0;
    GeoIntegralPropertyRefPtr inds_n = 0;
    VRGeometry* geo = 0;

    Vec3f vmin, vmax;
    float r = 0;
    bool vmm_changed = false;

    //void init(vector<VRGeometry*>& geos, VRMaterial* mat) {
    void init(vector<Geo>& geos, VRMaterial* mat) {
        geo = new VRGeometry("factory_part"); // init new object

        pos = GeoPnt3fProperty::create();
        norms = GeoVec3fProperty::create();
        inds_p = GeoUInt32Property::create();
        inds_n = GeoUInt32Property::create();

        geo->setType(GL_TRIANGLES);
        geo->setPositions( pos );
        geo->setNormals( norms );
        geo->setIndices( inds_p );
        geo->getMesh()->setIndex(inds_n, Geometry::NormalsIndex);
        geo->setMaterial(mat);

        geos.push_back(*this);

        vmin = Vec3f(1e9, 1e9, 1e9);
        vmax = Vec3f(-1e9, -1e9, -1e9);

        Np = Nn = 0;
        r = 0;
    }

    bool inBB(Pnt3f& v) {
        if (vmm_changed) {
            Vec3f d = (vmax - vmin);
            for (int i=0; i<3; i++) r = max(r, d[i]);
        }

        for (int i=0; i<3; i++) if (v[i] > vmax[i]+r || v[i] < vmin[i]-r) return false;
        return true;
    }

    void updateBB(Pnt3f& v) {
        for (int i=0; i<3; i++) {
            vmin[i] = min(vmin[i], v[i]);
            vmax[i] = max(vmax[i], v[i]);
        }
        vmm_changed = true;
    }

    void updateN() {
        if (pos) Np = pos->size();
        if (norms) Nn = norms->size();
    }
};


VRObject* VRAMLLoader::load(string path) {
    VRGeometry* geo = new VRGeometry("aml_root");
    cout << "load AML path: " << path << endl;

    ;

    return geo;
}

OSG_END_NAMESPACE;
