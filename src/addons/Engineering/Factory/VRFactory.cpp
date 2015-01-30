#include "VRFactory.h"
#include "core/utils/toString.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/VRLod.h"
#include "addons/Engineering/CSG/Octree/Octree.h"

#include <iostream>
#include <fstream>
#include <boost/filesystem.hpp>

#include <OpenSG/OSGSceneFileHandler.h>
#include <OpenSG/OSGGeoProperties.h>
#include <OpenSG/OSGGeometry.h>

#include <GL/gl.h>

using namespace OSG;
using namespace boost::filesystem;

VRFactory::VRFactory() {}

void parsePoints(int& i, vector<string>& data, GeoVectorPropertyRecPtr res) {
    stringstream ss;
    for (; data[i][0] != ']' and i < data.size(); i++) ss << data[i];

    Pnt3f v;
    while(ss) {
        ss >> v[0];
        ss >> v[1];
        ss >> v[2];
        ss.get();
        res->addValue( v );
    }
}

void parseNorms(int& i, vector<string>& data, GeoVectorPropertyRecPtr res) {
    stringstream ss;
    for (; data[i][0] != ']' and i < data.size(); i++) ss << data[i];

    Vec3f v;
    while(ss) {
        ss >> v[0];
        ss >> v[1];
        ss >> v[2];
        ss.get();
        res->addValue( v );
    }
}

void parseInds(int& i, vector<string>& data, GeoIntegralPropertyRefPtr res, int offset) {
    stringstream ss;
    for (; data[i][0] != ']' and i < data.size(); i++) ss << data[i];

    int v;
    while(ss) {
        ss >> v;
        ss.get();
        if (v >= 0) res->addValue( offset+v );
    }
}

void finalizeObject(vector<string>& data, vector<VRGeometry*>& geos) {
    if (data.size() < 10) return;

    static Vec3f last_dc(-1,-1,-1);
    static VRGeometry* geo = 0;

    Vec3f dc = toVec3f( data[6].substr(12) ); // get diffuse color
    if (last_dc != dc) {
        // init new object
        geo = new VRGeometry("factory_part");

        GeoPnt3fPropertyRecPtr pos = GeoPnt3fProperty::create();
        GeoVec3fPropertyRecPtr norms = GeoVec3fProperty::create();
        GeoUInt32PropertyRecPtr inds_p = GeoUInt32Property::create();
        GeoUInt32PropertyRecPtr inds_n = GeoUInt32Property::create();

        geo->setType(GL_TRIANGLES);
        geo->setPositions( pos );
        geo->setNormals( norms );
        geo->setIndices( inds_p );
        geo->getMesh()->setIndex(inds_n, Geometry::NormalsIndex);

        VRMaterial* mat = new VRMaterial("fmat");
        mat->setDiffuse(dc);
        geo->setMaterial(mat);

        geos.push_back(geo);
        last_dc = dc;
    }

    int i = 21;
    GeoVectorPropertyRecPtr pos = geo->getMesh()->getPositions();
    GeoVectorPropertyRecPtr norms = geo->getMesh()->getNormals();
    GeoIntegralPropertyRefPtr inds_p = geo->getMesh()->getIndices();
    GeoIntegralPropertyRefPtr inds_n = geo->getMesh()->getIndex(Geometry::NormalsIndex);

    int Np = pos->size();
    int Nn = norms->size();
    parsePoints(i, data, pos); i+=4;
    parseNorms(i, data, norms); i+=3;
    parseInds(i, data, inds_p, Np); i+=2;
    while (data[i][0] != ']') i++; i+=2;
    parseInds(i, data, inds_n, Nn);
}

VRObject* VRFactory::loadVRML(string path) { // wrl filepath
    ifstream file(path);
    if (!file.is_open()) { cout << "file " << path << " not found" << endl; return 0; }

    string header = "#VRML V2.0 utf8\n";
    string delimiter = "Transform";
    string dir = "split/";

    ofstream out;
    vector<string> data;
    string line;

    boost::filesystem::path d(dir);
    if (!exists(d)) create_directory(d);

    vector<VRGeometry*> geos;

    for ( int i=0; getline(file, line); ) {
        if ( line.compare(0, delimiter.size(), delimiter) == 0 ) {
            finalizeObject(data, geos);
            data.clear();
            i++;
        }

        data.push_back(line);
    }

    finalizeObject(data, geos);
    file.close();
    cout << "loaded " << geos.size() << " geometries" << endl;

    VRObject* res = new VRObject("factory");
    res->addAttachment("dynamicaly_generated", 0);

    for (auto g : geos) {
        res->addChild(g);

        GeoUInt32PropertyRecPtr Length = GeoUInt32Property::create();
        Length->addValue(g->getMesh()->getIndices()->size());
        g->setLengths(Length);
    }

    return res;
}

VRObject* VRFactory::setupLod(vector<string> paths) {
    vector<VRObject*> objects;
    for (auto p : paths) objects.push_back( loadVRML(p) );
    Vec3f p, bb1, bb2, sbb1, sbb2;
    Vec3i spaced;
    int spaceN;

    commitChanges();
    cout << "setupLod - changes commited\n";
    if (objects.size() == 0) return 0;

    VRObject* root = new VRObject("factory_lod_root");

    Octree tree(0.001);
    vector<VRLod*> grid;
    for (int i = 0; i<objects.size(); i++) {

        if (i == 0) {// get the full space to organise with LODs
            objects[i]->getBoundingBox(sbb1, sbb2);

            for (int j=0; j<3; j++) spaced[j] = ceil(sbb2[j]-sbb1[j]);
            spaceN = spaced[0]*spaced[1]*spaced[2];
            cout << "setupLod - space " << spaced << endl;

            for (int j=0; j<spaceN; j++) {
                VRLod* lod = new VRLod("factory_lod");
                lod->switchParent(root);
                grid.push_back( lod );
            }

            for (int u = 0; u<spaced[0]; u++) {
                for (int v = 0; v<spaced[1]; v++) {
                    for (int w = 0; w<spaced[2]; w++) {
                        int j = u*spaced[2]*spaced[1] + v*spaced[2] + w;
                        Vec3f pos = sbb1 + Vec3f(u+0.5, v+0.5, w*0.5);
                        grid[j]->setCenter(pos);
            }}}
        }

        for (auto _g : objects[i]->getChildren(true, "Geometry")) {
            VRGeometry* g = (VRGeometry*)_g;
            g->getBoundingBox(bb1, bb2);
            p = (bb1+bb2)*0.5 - sbb1;

            int j = floor(p[0])*spaced[2]*spaced[1] + floor(p[1])*spaced[2] + floor(p[2]);
            VRLod* lod = grid[j];
            while (i >= lod->getChildrenCount()) {
                lod->addChild(new VRObject("factory_lod_part"));
                lod->setDistance(i, i+0.5);
            }
            VRObject* anchor = lod->getChild(i);
            g->switchParent(anchor);

            /*if (i == 0) tree.add(p[0], p[1], p[2], g); // put the first file into an octree
            else {
                vector<void*> res = tree.radiusSearch(p[0], p[1], p[2], 0.01);
                cout << "radius search " << p << " found " << res.size() << endl;
            }*/
        }

        for (int j=0; j<spaceN; j++) {
            grid[j]->addChild(new VRObject("factory_empty_part"));
            int N = grid[j]->getChildrenCount();
            if (N > 1) grid[j]->setDistance(N-2, 2*N);
        }
    }

    return root;
}
