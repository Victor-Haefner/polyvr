#include "VRPrimitive.h"
#include "VRGeometry.h"
#include "OSGGeometry.h"
#include "VRGeoData.h"
#include "core/objects/material/VRMaterial.h"
#include <math.h>
#include <OpenSG/OSGSimpleGeometry.h>
#include <OpenSG/OSGGeoProperties.h>
#include <OpenSG/OSGSimpleMaterial.h>
#include <OpenSG/OSGNameAttachment.h>

using namespace OSG;

int VRPrimitive::getNParams() { return N; }
string VRPrimitive::getType() { return type; }
void VRPrimitive::fromString(string s) { stringstream ss(s); fromStream(ss); }
string VRPrimitive::toString() { stringstream ss; toStream(ss); return ss.str(); }
shared_ptr<VRPrimitive> VRPrimitive::create(string p) {
    if (p == "Plane") return shared_ptr<VRPrimitive>( new VRPlane() );
    if (p == "Box") return shared_ptr<VRPrimitive>( new VRBox() );
    if (p == "Sphere") return shared_ptr<VRPrimitive>( new VRSphere() );
    if (p == "Torus") return shared_ptr<VRPrimitive>( new VRTorus() );
    if (p == "Teapot") return shared_ptr<VRPrimitive>( new VRTeapot() );
    if (p == "Cylinder") return shared_ptr<VRPrimitive>( new VRCylinder() );
    if (p == "Cone") return shared_ptr<VRPrimitive>( new VRCone() );
    if (p == "Arrow") return shared_ptr<VRPrimitive>( new VRArrow() );
    if (p == "Pill") return shared_ptr<VRPrimitive>( new VRPill() );
    if (p == "Gear") return shared_ptr<VRPrimitive>( new VRGear() );
    if (p == "Thread") return shared_ptr<VRPrimitive>( new VRScrewThread() );
    if (p == "Disk") return shared_ptr<VRPrimitive>( new VRDisk() );
    if (p == "Annulus") return shared_ptr<VRPrimitive>( new VRAnnulus() );
    return 0;
}

shared_ptr<VRPrimitive> VRPrimitive::copy() {
    auto p = create(getType());
    p->fromString(toString());
    return p;
}

vector<string> VRPrimitive::getTypes() {
    static bool init = false;
    static vector<string> prims;

    if (!init) {
        init = true;
        prims.push_back("Plane");
        prims.push_back("Box");
        prims.push_back("Sphere");
        prims.push_back("Cylinder");
        prims.push_back("Cone");
        prims.push_back("Torus");
        prims.push_back("Teapot");
        prims.push_back("Arrow");
        prims.push_back("Pill");
        prims.push_back("Gear");
        prims.push_back("Thread");
    }

    return prims;
}

vector<string> VRPrimitive::getTypeParameter(string type) {
    static bool init = false;
    static map<string, vector<string> > params;

    if (!init) {
        init = true;
        params["Plane"] = vector<string>();
        params["Box"] = vector<string>();
        params["Sphere"] = vector<string>();
        params["Cylinder"] = vector<string>();
        params["Cone"] = vector<string>();
        params["Torus"] = vector<string>();
        params["Teapot"] = vector<string>();
        params["Gear"] = vector<string>();
        params["Thread"] = vector<string>();

        params["Plane"].push_back("Size x");
        params["Plane"].push_back("Size y");
        params["Plane"].push_back("Segments x");
        params["Plane"].push_back("Segments y");

        params["Box"].push_back("Size x");
        params["Box"].push_back("Size y");
        params["Box"].push_back("Size z");
        params["Box"].push_back("Segments x");
        params["Box"].push_back("Segments y");
        params["Box"].push_back("Segments z");

        params["Sphere"].push_back("Radius");
        params["Sphere"].push_back("Iterations");

        params["Cylinder"].push_back("Height");
        params["Cylinder"].push_back("Radius");
        params["Cylinder"].push_back("Sides");
        params["Cylinder"].push_back("Do bottom");
        params["Cylinder"].push_back("Do top");
        params["Cylinder"].push_back("Do sides");

        params["Pill"].push_back("Height");
        params["Pill"].push_back("Radius");
        params["Pill"].push_back("Sides");
        params["Pill"].push_back("Do bottom");
        params["Pill"].push_back("Do top");
        params["Pill"].push_back("Do sides");

        params["Cone"].push_back("Height");
        params["Cone"].push_back("Radius");
        params["Cone"].push_back("Sides");
        params["Cone"].push_back("Do bottom");
        params["Cone"].push_back("DBoxo sides");

        params["Torus"].push_back("Inner radius");
        params["Torus"].push_back("Outer radius");
        params["Torus"].push_back("Segments");
        params["Torus"].push_back("Rings");

        params["Teapot"].push_back("Iterations");
        params["Teapot"].push_back("Scale");

        params["Arrow"].push_back("Height");
        params["Arrow"].push_back("Width");
        params["Arrow"].push_back("Trunc");
        params["Arrow"].push_back("Hat");
        params["Arrow"].push_back("Thickness");

        params["Gear"].push_back("Width");
        params["Gear"].push_back("Hole");
        params["Gear"].push_back("Pitch");
        params["Gear"].push_back("Number of teeth");
        params["Gear"].push_back("Teeth size");
        params["Gear"].push_back("Bevel");

        params["Thread"].push_back("Length");
        params["Thread"].push_back("Radius");
        params["Thread"].push_back("Pitch");
        params["Thread"].push_back("Segments");
    }

    if (params.count(type)) return params[type];
    else return vector<string>();
}

VRPlane::VRPlane() { N = 4; type = "Plane"; }
VRBox::VRBox() { N = 6; type = "Box"; }
VRSphere::VRSphere() { N = 2; type = "Sphere"; }
VRTorus::VRTorus() { N = 4; type = "Torus"; }
VRTeapot::VRTeapot() { N = 2; type = "Teapot"; }
VRCylinder::VRCylinder() { N = 6; type = "Cylinder"; }
VRCone::VRCone() { N = 5; type = "Cone"; }
VRArrow::VRArrow() { N = 5; type = "Arrow"; }
VRPill::VRPill() { N = 6; type = "Pill"; }
VRScrewThread::VRScrewThread() { N = 4; type = "Thread"; }
VRGear::VRGear() { N = 6; type = "Gear"; }
VRDisk::VRDisk() { N = 2; type = "Disk"; }
VRAnnulus::VRAnnulus() { N = 3; type = "Annulus"; }

VRGear::VRGear(float w, float h, float p, int Nt, float St, float b) {
    width = w;
    hole = h;
    pitch = p;
    teeth_size = St;
    bevel = b;
    teeth_number = Nt;
    N = 6; type = "Gear";
}

void VRPlane::fromStream(stringstream& ss) { ss >> width >> height >> Nx >> Ny; }
void VRBox::fromStream(stringstream& ss) { ss >> width >> height >> depth >> Nx >> Ny >> Nz; }
void VRSphere::fromStream(stringstream& ss) { ss >> radius >> iterations; }
void VRTorus::fromStream(stringstream& ss) { ss >> inner_radius >> outer_radius >> Nsegments >> Nrings; }
void VRTeapot::fromStream(stringstream& ss) { ss >> iterations >> scale; }
void VRCylinder::fromStream(stringstream& ss) { ss >> height >> radius >> Nsides >> doTop >> doBottom >> doSides; }
void VRPill::fromStream(stringstream& ss) { ss >> height >> radius >> Nsides >> doTop >> doBottom >> doSides; }
void VRCone::fromStream(stringstream& ss) { ss >> height >> radius >> Nsides >> doBottom >> doSides; }
void VRArrow::fromStream(stringstream& ss) { ss >> height >> width >> trunc >> hat >> thickness; }
void VRScrewThread::fromStream(stringstream& ss) { ss >> length >> radius >> pitch >> Nsegments; }
void VRGear::fromStream(stringstream& ss) { ss >> width >> hole >> pitch >> teeth_number >> teeth_size >> bevel; }
void VRDisk::fromStream(stringstream& ss) { ss >> radius >> Nsegments; }
void VRAnnulus::fromStream(stringstream& ss) { ss >> outerRadius >> innerRadius >> Nsegments; }

void VRPlane::toStream(stringstream& ss) { ss << width << " " << height << " " << Nx << " " << Ny; }
void VRBox::toStream(stringstream& ss) { ss << width << " " << height << " " << depth << " " << Nx << " " << Ny << " " << Nz; }
void VRSphere::toStream(stringstream& ss) { ss << radius << " " << iterations; }
void VRTorus::toStream(stringstream& ss) { ss << inner_radius << " " << outer_radius << " " << Nsegments << " " << Nrings; }
void VRTeapot::toStream(stringstream& ss) { ss << iterations << " " << scale; }
void VRCylinder::toStream(stringstream& ss) { ss << height << " " << radius << " " << Nsides << " " << doTop << " " << doBottom << " " << doSides; }
void VRPill::toStream(stringstream& ss) { ss << height << " " << radius << " " << Nsides << " " << doTop << " " << doBottom << " " << doSides; }
void VRCone::toStream(stringstream& ss) { ss << height << " " << radius << " " << Nsides << " " << doBottom << " " << doSides; }
void VRArrow::toStream(stringstream& ss) { ss << height << " " << width << " " << trunc << " " << hat << " " << thickness; }
void VRScrewThread::toStream(stringstream& ss) { ss << length << " " << radius << " " << pitch << " " << Nsegments; }
void VRGear::toStream(stringstream& ss) { ss << width << " " << hole << " " << pitch << " " << teeth_number << " " << teeth_size << " " << bevel; }
void VRDisk::toStream(stringstream& ss) { ss << radius << Nsegments; }
void VRAnnulus::toStream(stringstream& ss) { ss << outerRadius << innerRadius << Nsegments; }

GeometryMTRecPtr VRPlane::make() {
    //return makePlaneGeo(width, height, Nx, Ny);
    VRGeoData data;

    Pnt3d o = -Pnt3d(width, height, 0)*0.5;
    Vec3d s = Vec3d(width/Nx, height/Ny, 1);

    map<Vec2i, int> pIDs;
    map<Vec2i, int> nIDs;
    map<Vec2i, int> tIDs;

    for (int i=0; i<=Nx; i++) {
        for (int j=0; j<=Ny; j++) {
            pIDs[Vec2i(i,j)] = data.pushPos( o + Vec3d(s[0]*i,s[1]*j,0) );
            nIDs[Vec2i(i,j)] = data.pushNorm(Vec3d(0,0,-1));
            tIDs[Vec2i(i,j)] = data.pushTexCoord(Vec2d(float(i)/Nx,float(j)/Ny));
        }
    }

    auto pushQuad = [&](Vec2i I1, Vec2i I2, Vec2i I3, Vec2i I4) {
        data.pushQuad( pIDs[I1], pIDs[I2], pIDs[I3], pIDs[I4] );
    };

    for (int i=0; i<Nx; i++) {
        for (int j=0; j<Ny; j++) {
            pushQuad(Vec2i(i,j), Vec2i(i,j+1), Vec2i(i+1,j+1), Vec2i(i+1,j));
        }
    }

    auto geo = data.asGeometry("Plane");
    return geo->getMesh()->geo;
}

GeometryMTRecPtr VRBox::make() {
    //return makeBoxGeo(width, height, depth, Nx, Ny, Nz);
    VRGeoData data;

    Pnt3d o = -Pnt3d(width, height, depth)*0.5;
    Vec3d s = Vec3d(width/Nx, height/Ny, depth/Nz);

    map<Vec3i, int> pIDs;
    map<Vec4i, int> nIDs;
    map<Vec4i, int> tIDs;

    auto spaceSkip = [&](int& i, int& j, int& k) {
        if (i > 0 && i < Nx)
            if (j > 0 && j < Ny)
                if (k == 1) k = Nz;
    };

    for (int i=0; i<=Nx; i++) {
        for (int j=0; j<=Ny; j++) {
            for (int k=0; k<=Nz; k++) {
                spaceSkip(i,j,k);

                if (i == 0 || i == Nx || j == 0 || j == Ny || k == 0 || k == Nz)
                    pIDs[Vec3i(i,j,k)] = data.pushPos( o + Vec3d(s[0]*i,s[1]*j,s[2]*k) );

                if (i == 0 ) nIDs[Vec4i(i,j,k,0)] = data.pushNorm(Vec3d(-1,0,0));
                if (i == Nx) nIDs[Vec4i(i,j,k,0)] = data.pushNorm(Vec3d( 1,0,0));
                if (j == 0 ) nIDs[Vec4i(i,j,k,1)] = data.pushNorm(Vec3d(0,-1,0));
                if (j == Ny) nIDs[Vec4i(i,j,k,1)] = data.pushNorm(Vec3d(0, 1,0));
                if (k == 0 ) nIDs[Vec4i(i,j,k,2)] = data.pushNorm(Vec3d(0,0,-1));
                if (k == Nz) nIDs[Vec4i(i,j,k,2)] = data.pushNorm(Vec3d(0,0, 1));

                if (i == 0 ) tIDs[Vec4i(i,j,k,0)] = data.pushTexCoord(Vec2d(float(k)/Nz,float(j)/Ny));
                if (i == Nx) tIDs[Vec4i(i,j,k,0)] = data.pushTexCoord(Vec2d(float(k)/Nz,float(j)/Ny));
                if (j == 0 ) tIDs[Vec4i(i,j,k,1)] = data.pushTexCoord(Vec2d(float(i)/Nx,float(k)/Nz));
                if (j == Ny) tIDs[Vec4i(i,j,k,1)] = data.pushTexCoord(Vec2d(float(i)/Nx,float(k)/Nz));
                if (k == 0 ) tIDs[Vec4i(i,j,k,2)] = data.pushTexCoord(Vec2d(float(i)/Nx,float(j)/Ny));
                if (k == Nz) tIDs[Vec4i(i,j,k,2)] = data.pushTexCoord(Vec2d(float(i)/Nx,float(j)/Ny));
            }
        }
    }

    auto pushQuad = [&](Vec3i I1, Vec3i I2, Vec3i I3, Vec3i I4, int d) {
#ifdef WASM
        data.pushTri( pIDs[I1], pIDs[I2], pIDs[I3] );
        data.pushTri( pIDs[I1], pIDs[I3], pIDs[I4] );
        data.pushNormalIndex( nIDs[Vec4i(I1[0], I1[1], I1[2], d)] );
        data.pushNormalIndex( nIDs[Vec4i(I2[0], I2[1], I2[2], d)] );
        data.pushNormalIndex( nIDs[Vec4i(I3[0], I3[1], I3[2], d)] );
        data.pushNormalIndex( nIDs[Vec4i(I1[0], I1[1], I1[2], d)] );
        data.pushNormalIndex( nIDs[Vec4i(I3[0], I3[1], I3[2], d)] );
        data.pushNormalIndex( nIDs[Vec4i(I4[0], I4[1], I4[2], d)] );
        data.pushTexCoordIndex( tIDs[Vec4i(I1[0], I1[1], I1[2], d)] );
        data.pushTexCoordIndex( tIDs[Vec4i(I2[0], I2[1], I2[2], d)] );
        data.pushTexCoordIndex( tIDs[Vec4i(I3[0], I3[1], I3[2], d)] );
        data.pushTexCoordIndex( tIDs[Vec4i(I1[0], I1[1], I1[2], d)] );
        data.pushTexCoordIndex( tIDs[Vec4i(I3[0], I3[1], I3[2], d)] );
        data.pushTexCoordIndex( tIDs[Vec4i(I4[0], I4[1], I4[2], d)] );
#else
        data.pushQuad( pIDs[I1], pIDs[I2], pIDs[I3], pIDs[I4] );
        data.pushNormalIndex( nIDs[Vec4i(I1[0], I1[1], I1[2], d)] );
        data.pushNormalIndex( nIDs[Vec4i(I2[0], I2[1], I2[2], d)] );
        data.pushNormalIndex( nIDs[Vec4i(I3[0], I3[1], I3[2], d)] );
        data.pushNormalIndex( nIDs[Vec4i(I4[0], I4[1], I4[2], d)] );
        data.pushTexCoordIndex( tIDs[Vec4i(I1[0], I1[1], I1[2], d)] );
        data.pushTexCoordIndex( tIDs[Vec4i(I2[0], I2[1], I2[2], d)] );
        data.pushTexCoordIndex( tIDs[Vec4i(I3[0], I3[1], I3[2], d)] );
        data.pushTexCoordIndex( tIDs[Vec4i(I4[0], I4[1], I4[2], d)] );
#endif
    };

    for (int i=0; i<=Nx; i++) {
        for (int j=0; j<=Ny; j++) {
            for (int k=0; k<=Nz; k++) {
                spaceSkip(i,j,k);

                if (i == 0 ) if (j < Ny && k < Nz) pushQuad(Vec3i(i,j,k), Vec3i(i,j,k+1), Vec3i(i,j+1,k+1), Vec3i(i,j+1,k), 0);
                if (i == Nx) if (j < Ny && k < Nz) pushQuad(Vec3i(i,j,k), Vec3i(i,j+1,k), Vec3i(i,j+1,k+1), Vec3i(i,j,k+1), 0);
                if (j == 0 ) if (i < Nx && k < Nz) pushQuad(Vec3i(i,j,k), Vec3i(i+1,j,k), Vec3i(i+1,j,k+1), Vec3i(i,j,k+1), 1);
                if (j == Ny) if (i < Nx && k < Nz) pushQuad(Vec3i(i,j,k), Vec3i(i,j,k+1), Vec3i(i+1,j,k+1), Vec3i(i+1,j,k), 1);
                if (k == 0 ) if (i < Nx && j < Ny) pushQuad(Vec3i(i,j,k), Vec3i(i,j+1,k), Vec3i(i+1,j+1,k), Vec3i(i+1,j,k), 2);
                if (k == Nz) if (i < Nx && j < Ny) pushQuad(Vec3i(i,j,k), Vec3i(i+1,j,k), Vec3i(i+1,j+1,k), Vec3i(i,j+1,k), 2);
            }
        }
    }

    auto geo = data.asGeometry("Box");
    return geo->getMesh()->geo;
}

GeometryMTRecPtr VRCylinder::make() {
    //return makeCylinderGeo(height, radius, Nsides, doSides, doTop, doBottom);

    if (!doSides && !doTop && !doBottom) return 0;

    VRGeoData data;

    Pnt3d p1 = Pnt3d(0,-height*0.5,0);
    Pnt3d p2 = Pnt3d(0, height*0.5,0);

    vector<Vec3d> ring;
    for (int i=0; i<Nsides; i++) {
        double a = i*2*Pi/Nsides;
        ring.push_back( Vec3d(cos(a),0,sin(a))*radius );
    }

    if (doBottom) {
        int rb0 = -1;
        for (auto v : ring) {
            v[1] = -height*0.5;
            int vID = data.pushVert(Pnt3d(v), Vec3d(0,-1,0), Vec2d(0,0));
            if (rb0 < 0) rb0 = vID;
        }
        int i0 = data.pushVert(p1, Vec3d(0,-1,0), Vec2d(0,0));
        for (int i=0; i<Nsides; i++) data.pushTri(i0, rb0+i, rb0+(i+1)%Nsides);
    }

    if (doTop) {
        int rt0 = -1;
        for (auto v : ring) {
            v[1] = height*0.5;
            int vID = data.pushVert(Pnt3d(v), Vec3d(0,1,0), Vec2d(0,1));
            if (rt0 < 0) rt0 = vID;
        }
        int i0 = data.pushVert(p2, Vec3d(0,1,0), Vec2d(0,1));
        for (int i=0; i<Nsides; i++) data.pushTri(i0, rt0+(i+1)%Nsides, rt0+i);
    }

    if (doSides) {
        int rs0 = -1;
        for (int i=0; i<ring.size(); i++) {
            auto v = ring[i];
            double t = double(i)/(ring.size()-1);
            Vec3d n = v;
            n.normalize();
            v[1] = -height*0.5;
            int vID = data.pushVert(Pnt3d(v), n, Vec2d(t,0));
            if (rs0 < 0) rs0 = vID;
            v[1] =  height*0.5;
            data.pushVert(Pnt3d(v), n, Vec2d(t,1));
        }
        for (int i=0; i<Nsides; i++) data.pushQuad(rs0+2*i, rs0+2*i+1, rs0+2*((i+1)%Nsides)+1, rs0+2*((i+1)%Nsides));
    }

    auto geo = data.asGeometry("Cylinder");
    return geo->getMesh()->geo;
}

GeometryMTRecPtr VRSphere::make() { return makeSphereGeo(iterations, radius); }
GeometryMTRecPtr VRTorus::make() { return makeTorusGeo(inner_radius, outer_radius, Nsegments, Nrings); }
GeometryMTRecPtr VRTeapot::make() { return makeTeapotGeo(iterations, scale); }
GeometryMTRecPtr VRCone::make() { return makeConeGeo(height, radius, Nsides, doSides, doBottom); }

GeometryMTRecPtr VRArrow::make() {
    VRGeoData data;

    float w2 = width*0.5;

    auto pushArrow = [&](float t, Vec3d n) {
        int v1 = data.pushVert(Vec3d(0,t,0), n);
        int v2 = data.pushVert(Vec3d(-w2,t,hat), n);
        int v3 = data.pushVert(Vec3d(w2,t,hat), n);
        int v4 = data.pushVert(Vec3d(-trunc*0.5,t,hat), n);
        int v5 = data.pushVert(Vec3d(trunc*0.5,t,hat), n);
        int v6 = data.pushVert(Vec3d(-trunc*0.5,t,height), n);
        int v7 = data.pushVert(Vec3d(trunc*0.5,t,height), n);
        data.pushTri(v1,v2,v3);
        data.pushQuad(v4,v6,v7,v5);
    };

    auto pushRect = [&](float x0, float x1, float y0, float y1, float z0, float z1, Vec3d n) {
        int v1 = data.pushVert(Vec3d(x0,y0,z0), n);
        int v2 = data.pushVert(Vec3d(x1,y0,z1), n);
        int v3 = data.pushVert(Vec3d(x1,y1,z1), n);
        int v4 = data.pushVert(Vec3d(x0,y1,z0), n);
        data.pushQuad(v1,v2,v3,v4);
    };

    if (thickness == 0) pushArrow(0, Vec3d(0,1,0));
    else {
        float t2 = thickness*0.5;

        pushArrow(t2, Vec3d(0,1,0));
        pushArrow(-t2, Vec3d(0,1,0));

        Vec3d nh1(hat, 0, w2); nh1.normalize();
        Vec3d nh2(hat, 0, -w2); nh2.normalize();
        pushRect(0,-w2,t2,-t2,0,hat, nh1);
        pushRect(0,w2,t2,-t2,0,hat, nh2);
        pushRect(-w2,-trunc*0.5,t2,-t2,hat,hat, Vec3d(0,0,-1));
        pushRect(trunc*0.5,w2,t2,-t2,hat,hat, Vec3d(0,0,-1));
        pushRect(-trunc*0.5,trunc*0.5,t2,-t2,height,height, Vec3d(0,0,-1));
        pushRect(-trunc*0.5,-trunc*0.5,t2,-t2,hat,height, Vec3d(1,0,0));
        pushRect(trunc*0.5,trunc*0.5,t2,-t2,height,hat, Vec3d(-1,0,0));
    }

    auto geo = data.asGeometry("Arrow");
    return geo->getMesh()->geo;
}

GeometryMTRecPtr VRPill::make() {
    VRGeoData data;

    int RN = 0;

    auto pushRing = [&](float r, float h, float b) {
        for (int i=0; i<Nsides; i++) {
            float a = i*2.0*Pi/Nsides;
            Vec3d n = Vec3d(cos(a)*cos(b), sin(b), sin(a)*cos(b));
            Vec3d p = n*r + Vec3d(0, h, 0);
            data.pushVert(p, n);
        }
        return RN++;
    };

    auto pushRingInds = [&](int r1, int r2, int order) {
        int N1 = r1*Nsides;
        int N2 = r2*Nsides;
        for (int i=0; i<Nsides; i++) {
            int j = i+1; if (j == Nsides) j = 0;
            if (order == 1) data.pushQuad(i+N1, j+N1, j+N2, i+N2);
            else            data.pushQuad(i+N1, i+N2, j+N2, j+N1);
        }
    };

    auto fillRing = [&](int r, float h, float n, int order) {
        int N0 = data.pushVert(Vec3d(0,h,0), Vec3d(0,n,0));
        int N = r*Nsides;
        for (int i=0; i<Nsides; i++) {
            int j = i+1; if (j == Nsides) j = 0;
            if (order == 1) data.pushTri(i+N, j+N, N0);
            else            data.pushTri(j+N, i+N, N0);
        }
    };

    int R1, R2, RT, RB;
    R1 = R2 = RT = RB = 0;
    if (doSides || doTop   ) R1 = pushRing(radius, height*0.5, 0);
    if (doSides || doBottom) R2 = pushRing(radius,-height*0.5, 0);
    if (doSides) pushRingInds(R1, R2, 1);

    if (doTop) {
        for (int i=1; i<Nsides; i++) {
            float b = i*0.5*Pi/Nsides;
            RT = pushRing(radius, height*0.5, b);
            if (i == 1) pushRingInds(R1,  RT,-1);
            else        pushRingInds(RT-1,RT,-1);
        }
    }

    if (doBottom) {
        for (int i=1; i<Nsides; i++) {
            float b = i*0.5*Pi/Nsides;
            RB = pushRing(radius, -height*0.5, -b);
            if (i == 1) pushRingInds(R2,  RB, 1);
            else        pushRingInds(RB-1,RB, 1);
        }
    }

    if (doTop)    fillRing(RT, height*0.5+radius, 1,-1);
    if (doBottom) fillRing(RB,-height*0.5-radius,-1, 1);

    auto geo = data.asGeometry("Pill");
    return geo->getMesh()->geo;
}

GeometryMTRecPtr VRScrewThread::make() {
    GeoUInt8PropertyMTRecPtr      Type = GeoUInt8Property::create();
    GeoUInt32PropertyMTRecPtr     Length = GeoUInt32Property::create();
    GeoPnt3fPropertyMTRecPtr      Pos = GeoPnt3fProperty::create();
    GeoVec3fPropertyMTRecPtr      Norms = GeoVec3fProperty::create();
    GeoUInt32PropertyMTRecPtr     Indices = GeoUInt32Property::create();
    SimpleMaterialMTRecPtr        Mat = SimpleMaterial::create();

    int rN = Nsegments;

    //positionen und Normalen
    Vec3d n;
    int iN = 0;
    int tN = round(length/pitch);
    float lT = 0.5*pitch/tan(Pi/6);
    float r1 = radius-lT*0.5;
    float r2 = radius+lT*0.5;
    for(int i=0; i<tN; i++) {
        iN = Pos->size();
        for (int j=0; j<rN; j++) {
            float sa = sin(j*2*Pi/rN);
            float ca = cos(j*2*Pi/rN);
            float o = j*pitch/rN;

            Pos->addValue(Vec3d(r1*ca ,r1*sa ,o+i*pitch));
            Pos->addValue(Vec3d(r2*ca ,r2*sa ,o+(i+0.5)*pitch));
            Norms->addValue(Vec3d(ca,sa,0));
            Norms->addValue(Vec3d(ca,sa,0));

            if (i == 0 && j == 0) continue;

            Indices->addValue(iN+2*j-2);
            Indices->addValue(iN+2*j);
            Indices->addValue(iN+2*j+1);
            Indices->addValue(iN+2*j-1);

            if (i == 0) continue;
            if (i == 1 && j == 0) continue;

            Indices->addValue(iN+2*j-2*rN-1);
            Indices->addValue(iN+2*j-2*rN+1);
            Indices->addValue(iN+2*j);
            Indices->addValue(iN+2*j-2);
        }
    }

    Type->addValue(GL_QUADS);
    Length->addValue(Indices->size()); // for each tooth 4 quads

    Mat->setDiffuse(Color3f(0.8,0.8,0.6));
    Mat->setAmbient(Color3f(0.4, 0.4, 0.2));
    Mat->setSpecular(Color3f(0.1, 0.1, 0.1));

    GeometryMTRecPtr geo = Geometry::create();
    geo->setTypes(Type);
    geo->setLengths(Length);
    geo->setIndices(Indices);
    geo->setPositions(Pos);
    geo->setNormals(Norms);
    geo->setMaterial(Mat);

    return geo;
}

GeometryMTRecPtr VRGear::make() {
    VRGeoData data;

    float r0 = hole;
    float r1 = radius();
    int tN = teeth_number;
    float ts = teeth_size;

    //positionen und Normalen
    r1 -= ts*0.5;
    float a[6], s[6], c[6];
    float z = width*0.5;
    float b = z*tan(bevel);
    float bt = ts*cos(bevel);
    float bz = -ts*sin(bevel);
    Vec3d n;
    int iN = 0;
    for(int i=0; i<tN; i++) {
        for (int j=0; j<6; j++) a[j] = 2*Pi*(i+j/6.)/tN;
        for (int j=0; j<6; j++) { c[j] = cos(a[j]); s[j] = sin(a[j]); }

        Vec3d n1 = Vec3d(0,0,-1);
        Vec3d n2 = Vec3d(0,0, 1);

        iN = data.size();
        data.pushVert(Vec3d(c[0]*r0, s[0]*r0, -z), n1); // 0
        data.pushVert(Vec3d(c[0]*(r1-b), s[0]*(r1-b), -z), n1); // 1
        data.pushVert(Vec3d(c[1]*(r1-b+bt*0.5), s[1]*(r1-b+bt*0.5), -z+bz*0.5), n1); // 2
        data.pushVert(Vec3d(c[2]*(r1-b+bt), s[2]*(r1-b+bt), -z+bz), n1); // 3
        data.pushVert(Vec3d(c[3]*(r1-b+bt), s[3]*(r1-b+bt), -z+bz), n1); // 4
        data.pushVert(Vec3d(c[4]*(r1-b+bt*0.5), s[4]*(r1-b+bt*0.5), -z+bz*0.5), n1); // 5
        data.pushVert(Vec3d(c[5]*(r1-b), s[5]*(r1-b), -z), n1); // 6
        data.pushVert(Vec3d(c[5]*r0, s[5]*r0, -z), n1); // 7

        data.pushVert(Vec3d(c[0]*r0, s[0]*r0, z), n2); // 8
        data.pushVert(Vec3d(c[0]*(r1+b), s[0]*(r1+b), z), n2); // 9
        data.pushVert(Vec3d(c[1]*(r1+b+bt*0.5), s[1]*(r1+b+bt*0.5), z+bz*0.5), n2); // 10
        data.pushVert(Vec3d(c[2]*(r1+b+bt), s[2]*(r1+b+bt), z+bz), n2); // 11
        data.pushVert(Vec3d(c[3]*(r1+b+bt), s[3]*(r1+b+bt), z+bz), n2); // 12
        data.pushVert(Vec3d(c[4]*(r1+b+bt*0.5), s[4]*(r1+b+bt*0.5), z+bz*0.5), n2); // 13
        data.pushVert(Vec3d(c[5]*(r1+b), s[5]*(r1+b), z), n2); // 14
        data.pushVert(Vec3d(c[5]*r0, s[5]*r0, z), n2); // 15

        data.pushQuad(iN+1, iN+0, iN+7, iN+6); // T1 unten
        data.pushQuad(iN+2, iN+1, iN+6, iN+5); // T2 unten
        data.pushQuad(iN+3, iN+2, iN+5, iN+4); // T3 unten
        data.pushQuad(iN+8, iN+9, iN+14, iN+15); // T1 unten
        data.pushQuad(iN+9, iN+10, iN+13, iN+14); // T2 unten
        data.pushQuad(iN+10, iN+11, iN+12, iN+13); // T3 unten

        if (i<tN-1) data.pushQuad(iN+6, iN+7, iN+16, iN+17); // N unten
        else data.pushQuad(iN+6, iN+7, 0, 1); // loop closing quad

        if (i<tN-1) data.pushQuad(iN+15, iN+14, iN+25, iN+24); // N oben
        else data.pushQuad(iN+15, iN+14, 9, 8); // loop closing quad
    }

    // sides
    int iNs = data.size();
    for(int i=0; i<tN; i++) {
        for (int j=0; j<6; j++) a[j] = 2*Pi*(i+j/6.)/tN;
        for (int j=0; j<6; j++) { c[j] = cos(a[j]); s[j] = sin(a[j]); }

        iN = data.size();
        n = Vec3d(-c[0], -s[0], 0); n.normalize();
        data.pushVert(Vec3d(c[0]*r0, s[0]*r0, -z), n); // 0
        n = Vec3d(c[0]+s[0], -c[0]+s[0], 0); n.normalize();
        data.pushVert(Vec3d(c[0]*(r1-b), s[0]*(r1-b), -z), n); // 1
        n = Vec3d(s[1], -c[1], 0); n.normalize();
        data.pushVert(Vec3d(c[1]*(r1-b+bt*0.5), s[1]*(r1-b+bt*0.5), -z+bz*0.5), n); // 2
        n = Vec3d(c[2]+s[2], -c[2]+s[2], 0);
        data.pushVert(Vec3d(c[2]*(r1-b+bt), s[2]*(r1-b+bt), -z+bz), n); // 3
        n = Vec3d(c[3]-s[3], c[3]+s[3], 0); n.normalize();
        data.pushVert(Vec3d(c[3]*(r1-b+bt), s[3]*(r1-b+bt), -z+bz), n); // 4
        n = Vec3d(-s[4], c[4], 0); n.normalize();
        data.pushVert(Vec3d(c[4]*(r1-b+bt*0.5), s[4]*(r1-b+bt*0.5), -z+bz*0.5), n); // 5
        n = Vec3d(c[5]-s[5], c[5]+s[5], 0); n.normalize();
        data.pushVert(Vec3d(c[5]*(r1-b), s[5]*(r1-b), -z), n); // 6
        n = Vec3d(-c[5], -s[5], 0); n.normalize();
        data.pushVert(Vec3d(c[5]*r0, s[5]*r0, -z), n); // 7

        n = Vec3d(-c[0], -s[0], 0); n.normalize();
        data.pushVert(Vec3d(c[0]*r0, s[0]*r0, z), n); // 8
        n = Vec3d(c[0]+s[0], -c[0]+s[0], 0); n.normalize();
        data.pushVert(Vec3d(c[0]*(r1+b), s[0]*(r1+b), z), n); // 9
        n = Vec3d(s[1], -c[1], 0); n.normalize();
        data.pushVert(Vec3d(c[1]*(r1+b+bt*0.5), s[1]*(r1+b+bt*0.5), z+bz*0.5), n); // 10
        n = Vec3d(c[2]+s[2], -c[2]+s[2], 0); n.normalize();
        data.pushVert(Vec3d(c[2]*(r1+b+bt), s[2]*(r1+b+bt), z+bz), n); // 11
        n = Vec3d(c[3]-s[3], c[3]+s[3], 0); n.normalize();
        data.pushVert(Vec3d(c[3]*(r1+b+bt), s[3]*(r1+b+bt), z+bz), n); // 12
        n = Vec3d(-s[4], c[4], 0); n.normalize();
        data.pushVert(Vec3d(c[4]*(r1+b+bt*0.5), s[4]*(r1+b+bt*0.5), z+bz*0.5), n); // 13
        n = Vec3d(c[5]-s[5], c[5]+s[5], 0); n.normalize();
        data.pushVert(Vec3d(c[5]*(r1+b), s[5]*(r1+b), z), n); // 14
        n = Vec3d(-c[5], -s[5], 0); n.normalize();
        data.pushVert(Vec3d(c[5]*r0, s[5]*r0, z), n); // 15

        data.pushQuad(iN+0, iN+8, iN+15, iN+7); // B1

        if (i<tN-1) data.pushQuad(iN+7, iN+15, iN+24, iN+16); // B2
        else data.pushQuad(iN+7, iN+15, iNs+8, iNs+0); // loop closing quad

        data.pushQuad(iN+9, iN+1, iN+2, iN+10);  // O1
        data.pushQuad(iN+10, iN+2, iN+3, iN+11); // O2
        data.pushQuad(iN+11, iN+3, iN+4, iN+12); // O3
        data.pushQuad(iN+12, iN+4, iN+5, iN+13); // O4
        data.pushQuad(iN+13, iN+5, iN+6, iN+14); // O5

        if (i<tN-1) data.pushQuad(iN+14, iN+6, iN+17, iN+25); // O6
        else data.pushQuad(iN+14, iN+6, iNs+1, iNs+9); // loop closing quad
    }

    auto mat = VRMaterial::create("gearDefault");
    mat->setDiffuse(Color3f(0.8,0.8,0.6));
    mat->setAmbient(Color3f(0.4, 0.4, 0.2));
    mat->setSpecular(Color3f(0.1, 0.1, 0.1));

    auto geo = data.asGeometry("Gear");
    geo->setMaterial(mat);
    return geo->getMesh()->geo;
}

float VRGear::radius() { return 0.5*pitch*teeth_number/Pi; }

GeometryMTRecPtr VRDisk::make() {
    VRGeoData data;

    Vec3d n(0,1,0);

    data.pushVert(Pnt3d(), n, Vec2d(0.5,0.5));

    double k = 2*Pi/(Nsegments-1);
    for (int i=0; i<Nsegments; i++) {
        double a = k*i;
        Pnt3d p(cos(a), 0, sin(-a));
        if (i+1 < Nsegments) data.pushVert(p*radius, n, Vec2d(p[0]*0.5+0.5, p[2]*0.5+0.5));
        if (i > 0 && i+1 < Nsegments) data.pushTri(0, -2, -1);
        if (i+1 == Nsegments) data.pushTri(0, -1, 1);
    }

    auto mat = VRMaterial::create("gearDefault");
    mat->setDiffuse (Color3f(0.8, 0.8, 0.6));
    mat->setAmbient (Color3f(0.4, 0.4, 0.2));
    mat->setSpecular(Color3f(0.1, 0.1, 0.1));

    auto geo = data.asGeometry("Disk");
    geo->setMaterial(mat);
    return geo->getMesh()->geo;
}

GeometryMTRecPtr VRAnnulus::make() {
    VRGeoData data;

    if (innerRadius >= outerRadius) return 0;

    double h = innerRadius / outerRadius;

    Vec3d n(0,1,0);

    double k = 2*Pi/Nsegments;
    for (int i=0; i<=Nsegments; i++) {
        double a = k*i;
        Pnt3d p(cos(a), 0, sin(-a));
        if (i+1 <= Nsegments) {
            data.pushVert(p*innerRadius, n, Vec2d(p[0]*0.5*h+0.5, p[2]*0.5*h+0.5));
            data.pushVert(p*outerRadius, n, Vec2d(p[0]*0.5+0.5, p[2]*0.5+0.5));
        }
        if (i > 0 && i+1 <= Nsegments) data.pushQuad(-4, -3, -1, -2);
        if (i == Nsegments) data.pushQuad(-2, -1, 1, 0);
    }

    auto mat = VRMaterial::create("gearDefault");
    mat->setDiffuse (Color3f(0.8, 0.8, 0.6));
    mat->setAmbient (Color3f(0.4, 0.4, 0.2));
    mat->setSpecular(Color3f(0.1, 0.1, 0.1));

    auto geo = data.asGeometry("Disk");
    geo->setMaterial(mat);
    return geo->getMesh()->geo;
}
