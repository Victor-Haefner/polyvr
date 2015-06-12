#include "VRFactory.h"
#include "core/utils/toString.h"
#include "core/utils/VRProgress.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/VRLod.h"
#include "core/math/Octree.h"

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

VRObject* VRFactory::loadVRML(string path) { // wrl filepath
    ifstream file(path);
    if (!file.is_open()) { cout << "file " << path << " not found" << endl; return 0; }

    // get file size
    file.seekg(0, ios_base::end);
    size_t fileSize = file.tellg();
    file.seekg(0, ios_base::beg);
    VRProgress prog("load VRML " + path, fileSize);

    int state = 0;
    map<int, string> states;
    states[0] = "Transform "; // 0
    states[1] = "diffuseColor "; // 6
    states[2] = "coord "; // 21 +2
    states[3] = "normal "; // x +2
    states[4] = "coordIndex "; // x +1
    states[5] = "colorIndex "; // x +1
    states[6] = "normalIndex "; // x +1

    Vec3f color;
    Vec3f last_col(-1,-1,-1);

    Geo geo;

    Pnt3f v;
    Vec3f n;
    int i;

    //vector<VRGeometry*> geos;
    vector<Geo> geos;
    map<Vec3f, VRMaterial*> mats;
    bool new_obj = true;
    bool new_color = true;
    int li = 0;

    string line;
    while ( getline(file, line) ) {
        prog.update( line.size() );
        li++;

        for (auto d : states) {
            //if ( line[d.second.size()-1] != ' ') continue; // optimization
            if ( line.compare(0, d.second.size(), d.second) == 0) {
                //if (state != d.first) cout << "got on line " << li << ": " << states[d.first] << " instead of: " << states[state] << endl;
                switch (d.first) {
                    case 0: break;
                    case 1:
                        new_obj = true;
                        if (line.size() > 12) color = toVec3f( line.substr(12) );
                        if (mats.count(color) == 0) {
                            mats[color] = new VRMaterial("fmat");
                            mats[color]->setDiffuse(color);
                        }

                        if (color != last_col) {
                            new_color = true;
                            last_col = color;
                        }
                        break;
                    case 2:
                        geo.updateN();
                        break;
                    case 3: break;
                    case 4: break;
                    case 5: break;
                }
                state = d.first+1;
                if (state == 7) state = 0;
                break;
            }
        }

        if (line[0] != ' ') continue;
        if (state == 6) continue; // skip color indices

        stringstream ss(line);
        switch (state) {
            case 3:
                while(ss >> v[0] && ss >> v[1] && ss >> v[2] && ss.get()) {
                    if (!new_color && new_obj) new_obj = !geo.inBB(v); // strange artifacts!!
                    geo.updateBB(v);

                    if (new_obj) {
                        new_obj = false;
                        new_color = false;
                        geo.init(geos, mats[color]);
                    }

                    geo.pos->addValue(v);
                }
                break;
            case 4:
                while(ss >> n[0] && ss >> n[1] && ss >> n[2] && ss.get()) geo.norms->addValue( n );
                break;
            case 5:
                while(ss >> i && ss.get()) if (i >= 0) geo.inds_p->addValue( geo.Np + i );
                break;
            case 0:
                while(ss >> i && ss.get()) if (i >= 0) geo.inds_n->addValue( geo.Nn + i );
                break;
        }
    }

    file.close();
    cout << "\nloaded " << geos.size() << " geometries" << endl;

    VRObject* res = new VRObject("factory");
    res->setPersistency(0);

    for (auto g : geos) {
        //Vec3f d = g.vmax - g.vmin;
        //if (d.length() < 0.1) continue; // skip very small objects

        if (g.inds_n->size() != g.inds_p->size()) { // not happening
            cout << " wrong indices lengths: " << g.inds_p->size() << " " << g.inds_n->size() << endl;
            continue;
        }

        if (g.inds_p->size() == 0) { // not happening
            cout << " empty geo: " << g.inds_p->size() << " " << g.inds_n->size() << endl;
            continue;
        }

        res->addChild(g.geo);

        GeoUInt32PropertyRecPtr Length = GeoUInt32Property::create();
        Length->addValue(g.geo->getMesh()->getIndices()->size());
        g.geo->setLengths(Length);
    }

    cout << "\nloaded2 " << res->getChildrenCount() << " geometries" << endl;

    return res;
}

class VRLODSpace : public VRObject {
    private:
        map<Vec4i, VRLod*> lod_spaces;
        float scale = 3; // smaller means bigger clusters

        VRLod* getSpace(Vec4i p) {
            if (lod_spaces.count(p)) return lod_spaces[p];
            VRLod* l = new VRLod("lod_space");
            l->addChild(new VRObject("lod_entry"));
            l->addDistance(max(15*p[3]/scale, 1.0f));
            l->setCenter( Vec3f(p[0], p[1], p[2])/scale );
            addChild(l);
            lod_spaces[p] = l;
            return l;
        }

    public:
        VRLODSpace() : VRObject("VRLODSpace") {
            setPersistency(0);
        }

        void add(VRObject* g) {
            Vec3f c = g->getBBCenter()*scale;
            Vec4i p; for(int i=0; i<3; i++) p[i] = round(c[i]);
            p[3] = ceil(g->getBBMax()*scale);
            getSpace(p)->getChild(0)->addChild(g);
        }

        void finalize() {
            cout << "finalizing " << lod_spaces.size() << " lod spaces!" << endl;
            for (auto l : lod_spaces ) {
                l.second->addEmpty();

                // testing
                /*VRGeometry* obj = new VRGeometry("bla");
                Vec4i p = l.first;
                Vec3f c = Vec3f(p[0], p[1], p[2]);
                obj->setFrom(c);
                obj->setPrimitive("Box", "0.1 0.1 0.1 1 1 1");
                l.second->addChild(obj);*/
            }
        }
};

VRObject* VRFactory::setupLod(vector<string> paths) {
    vector<VRObject*> objects;
    for (auto p : paths) objects.push_back( loadVRML(p) );
    Vec3f p;

    commitChanges();
    cout << "setupLod - changes commited\n";
    if (objects.size() == 0) return 0;

    // use all geometry to create micro lods
    VRObject* root = new VRObject("factory_lod_root");
    root->setPersistency(0);
    vector<VRLod*> micro_lods;
    for (uint i = 0; i<objects.size(); i++) {
        vector<VRObject*> geos = objects[i]->getChildren(true, "Geometry");
        VRProgress prog("setup factory LODs ", geos.size());
        for (auto g : geos) {
            prog.update(1);

            VRLod* lod = new VRLod("factory_lod");
            lod->addChild(g);
            lod->addEmpty();
            lod->setCenter( g->getBBCenter() );
            lod->setDistance(0, max(g->getBBMax()*15, 1.0f));
            micro_lods.push_back(lod);
        }
    }
    commitChanges();

    // use the micro lods to create space lods
    VRLODSpace* lodspace = new VRLODSpace();
    for (auto l : micro_lods) {
        lodspace->add(l);
    }
    root->addChild(lodspace);
    lodspace->finalize();

    return root;
}
