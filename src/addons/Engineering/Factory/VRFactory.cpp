#include "VRFactory.h"
#include "core/utils/toString.h"
#include "core/utils/VRProgress.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/OSGGeometry.h"
#include "core/objects/material/VRMaterial.h"
#include "core/objects/VRLod.h"
#include "core/math/Octree.h"
#include "core/math/boundingbox.h"

#include <iostream>
#include <fstream>

#include <OpenSG/OSGSceneFileHandler.h>
#include <OpenSG/OSGGeoProperties.h>
#include <OpenSG/OSGGeometry.h>

using namespace OSG;
using namespace boost::filesystem;

VRFactory::VRFactory() {}

string repSpaces(string s) {
    for(auto it = s.begin(); it != s.end(); ++it) {
        if(*it == ' ') *it = '|';
    }
    return s;
}

struct Geo {
    int Np = 0;
    int Nn = 0;
    GeoVectorPropertyMTRecPtr pos = 0;
    GeoVectorPropertyMTRecPtr norms = 0;
    GeoIntegralPropertyMTRecPtr inds_p = 0;
    GeoIntegralPropertyMTRecPtr inds_n = 0;
    VRGeometryPtr geo = 0;

    Vec3d vmin, vmax;
    double r = 0;
    bool vmm_changed = false;

    //void init(vector<VRGeometryPtr>& geos, VRMaterialPtr mat) {
    void init(vector<Geo>& geos, VRMaterialPtr mat, string path, bool thread) {
        geo = VRGeometry::create("part"); // init new object
        geo->setMaterial(mat);

        pos = GeoPnt3fProperty::create();
        norms = GeoVec3fProperty::create();
        inds_p = GeoUInt32Property::create();
        inds_n = GeoUInt32Property::create();

        VRGeometry::Reference ref(VRGeometry::FILE, repSpaces(path) + " " + repSpaces(geo->getName()) + " SOLIDWORKS-VRML2 " + toString(thread));
        geo->setReference( ref );

        geos.push_back(*this);

        vmin = Vec3d(1e9, 1e9, 1e9);
        vmax = Vec3d(-1e9, -1e9, -1e9);

        Np = Nn = 0;
        r = 0;
    }

    void finalize() {
        geo->setType(GL_TRIANGLES);
        geo->setPositions( pos );
        geo->setNormals( norms );
        geo->setIndices( inds_p );
        geo->getMesh()->geo->setIndex(inds_n, Geometry::NormalsIndex);
    }

    bool inBB(Pnt3d& v) {
        if (vmm_changed) {
            Vec3d d = (vmax - vmin);
            for (int i=0; i<3; i++) r = max(r, d[i]);
        }

        for (int i=0; i<3; i++) if (v[i] > vmax[i]+r || v[i] < vmin[i]-r) return false;
        return true;
    }

    void updateBB(Pnt3d& v) {
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

shared_ptr<VRFactory> VRFactory::create() { return shared_ptr<VRFactory>(new VRFactory()); }

bool VRFactory::loadVRML(string path, VRProgressPtr progress, VRTransformPtr res, bool thread) { // wrl filepath
    ifstream file(path);
    if (!file.is_open()) { cout << "file " << path << " not found" << endl; return true; }

    // get file size
    file.seekg(0, ios_base::end);
    size_t fileSize = file.tellg();
    file.seekg(0, ios_base::beg);
    if (progress == 0) progress = VRProgress::create();
    progress->setup("load VRML " + path, fileSize);
    progress->reset();

    int state = 0;
    map<int, string> states;
    states[0] = "Transform "; // 0
    states[1] = "diffuseColor "; // 6
    states[2] = "coord "; // 21 +2
    states[3] = "normal "; // x +2
    states[4] = "coordIndex "; // x +1
    states[5] = "colorIndex "; // x +1
    states[6] = "normalIndex "; // x +1

    Color3f color;
    Color3f last_col(-1,-1,-1);

    Geo geo;

    Pnt3d v;
    Vec3d n;
    int i;

    //vector<VRGeometryPtr> geos;
    vector<Geo> geos;
    map<Color3f, VRMaterialPtr> mats;
    bool new_obj = true;
    bool new_color = true;
    int li = 0;

    string line;
    while ( getline(file, line) ) {
        progress->update( line.size() );
        li++;

        for (auto d : states) {
            //if ( line[d.second.size()-1] != ' ') continue; // optimization
            if ( line.compare(0, d.second.size(), d.second) == 0) {
                //if (state != d.first) cout << "got on line " << li << ": " << states[d.first] << " instead of: " << states[state] << endl;
                switch (d.first) {
                    case 0: break;
                    case 1:
                        new_obj = true;
                        if (line.size() > 12) color = toValue<Color3f>( line.substr(12) );
                        if (mats.count(color) == 0) {
                            mats[color] = VRMaterial::create("fmat");
                            mats[color]->setDiffuse(Vec3f(color));
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
                        geo.init(geos, mats[color], path, thread);
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
    if (geos.size() == 0) { progress->reset(); return false; }

    res->setName("factory");
    res->setPersistency(0);

    for (auto g : geos) {
        //Vec3d d = g.vmax - g.vmin;
        //if (d.length() < 0.1) continue; // skip very small objects

        if (g.inds_n->size() != g.inds_p->size()) { // not happening
            cout << " wrong indices lengths: " << g.inds_p->size() << " " << g.inds_n->size() << endl;
            continue;
        }

        if (g.inds_p->size() == 0) { // not happening
            cout << " empty geo: " << g.inds_p->size() << " " << g.inds_n->size() << endl;
            continue;
        }

        g.finalize();
        res->addChild(g.geo);

        GeoUInt32PropertyMTRecPtr Length = GeoUInt32Property::create();
        Length->addValue(g.geo->getMesh()->geo->getIndices()->size());
        g.geo->setLengths(Length);
    }

    cout << "\nloaded2 " << res->getChildrenCount() << " geometries" << endl;
    progress->finish();
    return true;
}

OSG_BEGIN_NAMESPACE;

class VRLODSpace : public VRObject {
    private:
        map<Vec4i, VRLodPtr> lod_spaces;
        float scale = 3; // smaller means bigger clusters

        VRLodPtr getSpace(Vec4i p) {
            if (lod_spaces.count(p)) return lod_spaces[p];
            VRLodPtr l = VRLod::create("lod_space");
            l->addChild( VRObject::create("lod_entry") );
            l->addDistance(max(15*p[3]/scale, 1.0f));
            l->setCenter( Vec3d(p[0], p[1], p[2])/scale );
            addChild(l);
            lod_spaces[p] = l;
            return l;
        }

    public:
        VRLODSpace() : VRObject("VRLODSpace") {
            setPersistency(0);
        }

        static VRLODSpacePtr create() { return VRLODSpacePtr(new VRLODSpace()); }

        void add(VRObjectPtr g) {
            auto bb = g->getBoundingbox();
            Vec3d c = bb->center()*scale;
            Vec4i p; for(int i=0; i<3; i++) p[i] = round(c[i]);
            p[3] = ceil(bb->radius()*scale);
            getSpace(p)->getChild(0)->addChild(g);
        }

        void finalize() {
            cout << "finalizing " << lod_spaces.size() << " lod spaces!" << endl;
            for (auto l : lod_spaces ) {
                l.second->addEmpty();

                // testing
                /*VRGeometryPtr obj = VRGeometry::create("bla");
                Vec4i p = l.first;
                Vec3d c = Vec3d(p[0], p[1], p[2]);
                obj->setFrom(c);
                obj->setPrimitive("Box", "0.1 0.1 0.1 1 1 1");
                l.second->addChild(obj);*/
            }
        }
};

OSG_END_NAMESPACE;

VRObjectPtr VRFactory::setupLod(vector<string> paths) {
    vector<VRObjectPtr> objects;
    for (auto p : paths) {
        auto res = VRTransform::create("factory");
        loadVRML(p,0,res);
        objects.push_back( res );
    }
    Vec3d p;

    commitChanges();
    cout << "setupLod - changes commited\n";
    if (objects.size() == 0) return 0;

    // use all geometry to create micro lods
    VRObjectPtr root = VRObject::create("factory_lod_root");
    root->setPersistency(0);
    vector<VRLodPtr> micro_lods;
    for (unsigned int i = 0; i<objects.size(); i++) {
        vector<VRObjectPtr> geos = objects[i]->getChildren(true, "Geometry");
        VRProgress prog("setup factory LODs ", geos.size());
        for (auto g : geos) {
            prog.update(1);

            auto bb = g->getBoundingbox();
            VRLodPtr lod = VRLod::create("factory_lod");
            lod->addChild(g);
            lod->addEmpty();
            lod->setCenter( bb->center() );
            lod->setDistance(0, max(bb->radius()*15, 1.0f));
            micro_lods.push_back(lod);
        }
    }
    commitChanges();

    // use the micro lods to create space lods
    VRLODSpacePtr lodspace = VRLODSpace::create();
    for (auto l : micro_lods) {
        lodspace->add(l);
    }
    root->addChild(lodspace);
    lodspace->finalize();

    return root;
}
