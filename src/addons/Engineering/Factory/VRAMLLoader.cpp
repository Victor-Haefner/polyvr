#include "VRAMLLoader.h"

#include <iostream>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

//#include <collada/dae.h>
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
    GeoVectorPropertyRecPtr tc1 = 0;
    GeoVectorPropertyRecPtr tc2 = 0;
    GeoVectorPropertyRecPtr tc3 = 0;
    GeoVectorPropertyRecPtr tc4 = 0;
    GeoVectorPropertyRecPtr tc5 = 0;
    GeoVectorPropertyRecPtr tc6 = 0;
    GeoVectorPropertyRecPtr tc7 = 0;

    GeoVectorPropertyRecPtr pos = 0;
    GeoVectorPropertyRecPtr norms = 0;
    GeoVectorPropertyRecPtr cols = 0;
    GeoIntegralPropertyRefPtr inds_p = 0;
    GeoIntegralPropertyRefPtr inds_n = 0;
    GeoIntegralPropertyRefPtr inds_c = 0;
    GeoIntegralPropertyRefPtr types = 0;
    GeoIntegralPropertyRefPtr lengths = 0;
    VRGeometry* geo = 0;
    VRMaterial* mat = 0;

    Geo(string name) {
        geo = new VRGeometry(name);
        mat = new VRMaterial(name);

        tc1 = GeoPnt3fProperty::create();
        tc2 = GeoPnt3fProperty::create();
        tc3 = GeoPnt3fProperty::create();
        tc4 = GeoPnt3fProperty::create();
        tc5 = GeoPnt3fProperty::create();
        tc6 = GeoPnt3fProperty::create();
        tc7 = GeoPnt3fProperty::create();

        pos = GeoPnt3fProperty::create();
        norms = GeoVec3fProperty::create();
        cols = GeoVec3fProperty::create();
        inds_p = GeoUInt32Property::create();
        inds_n = GeoUInt32Property::create();
        inds_c = GeoUInt32Property::create();
        types = GeoUInt32Property::create();
        lengths = GeoUInt32Property::create();
    }

    void finish() {
        geo->setTypes(types);
        geo->setLengths(lengths);
        geo->setPositions(pos);
        geo->setNormals(norms);
        geo->setColors(cols);
        geo->getMesh()->setIndex(inds_p, Geometry::PositionsIndex);
        geo->getMesh()->setIndex(inds_c, Geometry::ColorsIndex);
        geo->getMesh()->setIndex(inds_n, Geometry::NormalsIndex);
        geo->setTexCoords(tc1, 0);
        geo->setTexCoords(tc2, 1);
        geo->setTexCoords(tc3, 2);
        geo->setTexCoords(tc4, 3);
        geo->setTexCoords(tc5, 4);
        geo->setTexCoords(tc6, 5);
        geo->setTexCoords(tc7, 6);
        geo->setMaterial(mat);
    }
};


VRObject* VRAMLLoader::load(string path) {
    VRObject* root = new VRObject("aml_root");
    cout << "load AML path: " << path << endl;

    //   ----------   MESH -----------
    Geo geo(path);

    geo.pos->addValue(Vec3f(0,0,0)); // vertex positionen
    geo.pos->addValue(Vec3f(1,0,0)); // vertex positionen
    geo.pos->addValue(Vec3f(1,1,0)); // vertex positionen
    geo.pos->addValue(Vec3f(0,1,0)); // vertex positionen

    geo.norms->addValue(Vec3f(0,0,1));

    for (int i=0; i<4; i++) { // per vertex
        //geo.cols->addValue(Vec4f(0,0,1,1));
        geo.tc1->addValue(Vec2f(0,0));
    }

    for (int i=0; i<4; i++) {
        geo.inds_p->addValue(i);
        geo.inds_n->addValue(0);
        //geo.inds_c->addValue(i);
    }

    geo.types->addValue(GL_QUADS);
    geo.lengths->addValue(4);

    //   ----------   MATERIAL -----------
    geo.mat->setDiffuse(Vec3f(1,0,1));
    geo.mat->setAmbient(Vec3f(1,0,1));
    geo.mat->setSpecular(Vec3f(1,0,1));
    geo.mat->setTransparency(0.5);
    geo.mat->setShininess(0.5);

    geo.finish();

    //   ----------   TRANSFORMATION -----------
    Matrix m( 1, 0, 0, 0,
              0, 1, 0, 0,
              0, 0, 1, 0,
              0, 0, 0, 1
             );
    geo.geo->setMatrix(m);

    // ------------ PHYSICS -----------------
    auto phys = geo.geo->getPhysics();
    phys->setDynamic(true);
    phys->setShape("Convex");
    phys->setPhysicalized(true);

    root->addChild(geo.geo);
    return root;
}

OSG_END_NAMESPACE;
