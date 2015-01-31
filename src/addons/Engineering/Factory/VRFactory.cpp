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

class VRLODGrid {
    private:
        Vec3f smin;
        Vec3i dim;
        int N;
        float res;
        float dist;
        vector<VRLod*> grid;
        VRObject* root;

        int toIndex(Vec3i p) {
            return p[0]*dim[2]*dim[1] + p[1]*dim[2] + p[2];
        }

        int toIndex(Vec3f p) {
            Vec3i ip;
            for (int i=0; i<3; i++) ip[i] = floor(p[i]/res);
            return toIndex(ip);
        }

    public:
        VRLODGrid(Vec3f smin, Vec3f smax, float res) {
            this->smin = smin;
            this->res = res;

            root = new VRObject("lodgrid_root");

            for (int j=0; j<3; j++) dim[j] = ceil((smax[j]-smin[j])/res);
            N = dim[0]*dim[1]*dim[2];
            cout << "setupLodGrid - space " << dim << endl;

            for (int j=0; j<N; j++) {
                VRLod* lod = new VRLod("factory_lod");
                lod->switchParent(root);
                grid.push_back( lod );
            }

            for (int u = 0; u<dim[0]; u++) {
                for (int v = 0; v<dim[1]; v++) {
                    for (int w = 0; w<dim[2]; w++) {
                        Vec3f pos = smin + Vec3f(u+0.5, v+0.5, w*0.5)*res;
                        grid[ toIndex(Vec3i(u,v,w)) ]->setCenter(pos);
            }}}

            // lod distance for each space segment
            dist = res*5;

        }

        void finalize() {
            for (int j=0; j<N; j++) { // all lods get an empty object
                grid[j]->addChild(new VRObject("factory_empty_part"));
            }
        }

        void addObject(VRObject* g, int detail) {
            Vec3f v1, v2, p;
            g->getBoundingBox(v1, v2);
            p = (v1+v2)*0.5 - smin;

            int i = toIndex(p);
            if (i >= grid.size()) cout << "addObj " << p << " " << grid.size() << " " << i;
            VRLod* lod = grid[i];
            while (detail >= lod->getChildrenCount()) {
                lod->addChild(new VRObject("factory_lod_part"));
                lod->setDistance(detail, dist*(detail+1));
            }

            VRObject* anchor = lod->getChild(detail);
            g->switchParent(anchor);
        }

        VRObject* getRoot() { return root; }
};

class VRLODSystem {
    private:
        map<float, VRLODGrid*> grids;
        Vec3f smin,smax;
        VRObject* root;

    public:
        VRLODSystem() {
            root = new VRObject("VRLODSystem_Root");
            root->addAttachment("dynamicaly_generated", 0);
        }

        void addObject(VRObject* g, int detail) {
            Vec3f v1, v2, d;
            g->getBoundingBox(v1, v2);
            d = v2-v1;
            float r = 0; // get the size of the object to pass it to the right grid level
            for (int i=0; i<3; i++) r = max(r,d[i]);

            for (auto gr : grids) if (r < gr.first) { gr.second->addObject(g, detail); break; }
        }

        void finalize() {
            for (auto g : grids ) {
                g.second->finalize();
                root->addChild(g.second->getRoot());
            }
        }

        void addLevel(float res) { grids[res] = new VRLODGrid(smin, smax, res); }
        void initSpace(VRObject* space) { space->getBoundingBox(smin, smax); }
        VRObject* getRoot() { return root; }
};

VRObject* VRFactory::setupLod(vector<string> paths) {
    vector<VRObject*> objects;
    for (auto p : paths) objects.push_back( loadVRML(p) );
    Vec3f p;

    commitChanges();
    cout << "setupLod - changes commited\n";
    if (objects.size() == 0) return 0;

    Octree tree(0.001);
    VRLODSystem* lodsys = new VRLODSystem();

    lodsys->initSpace(objects[0]);
    lodsys->addLevel(64);
    lodsys->addLevel(4);
    lodsys->addLevel(2);
    lodsys->addLevel(1);
    lodsys->addLevel(0.5);
    //lodsys->addLevel(0.25);

    for (int i = 0; i<objects.size(); i++) {
        for (auto _g : objects[i]->getChildren(true, "Geometry")) {
            lodsys->addObject(_g, i);
        }
    }

    lodsys->finalize();
    return lodsys->getRoot();
}
