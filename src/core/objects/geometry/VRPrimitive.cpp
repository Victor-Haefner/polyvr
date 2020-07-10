#include "VRPrimitive.h"
#include "VRGeometry.h"
#include "OSGGeometry.h"
#include "VRGeoData.h"
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
VRPrimitive* VRPrimitive::create(string p) {
    if (p == "Plane") return new VRPlane();
    if (p == "Box") return new VRBox();
    if (p == "Sphere") return new VRSphere();
    if (p == "Torus") return new VRTorus();
    if (p == "Teapot") return new VRTeapot();
    if (p == "Cylinder") return new VRCylinder();
    if (p == "Cone") return new VRCone();
    if (p == "Arrow") return new VRArrow();
    if (p == "Pill") return new VRPill();
    if (p == "Gear") return new VRGear();
    if (p == "Thread") return new VRScrewThread();
    return 0;
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

GeometryMTRecPtr VRSphere::make() { return makeSphereGeo(iterations, radius); }
GeometryMTRecPtr VRTorus::make() { return makeTorusGeo(inner_radius, outer_radius, Nsegments, Nrings); }
GeometryMTRecPtr VRTeapot::make() { return makeTeapotGeo(iterations, scale); }
GeometryMTRecPtr VRCylinder::make() { return makeCylinderGeo(height, radius, Nsides, doSides, doTop, doBottom); }
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
    float r1 = radius;
    float r2 = radius+0.5*pitch/tan(Pi/6);
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
    GeoUInt8PropertyMTRecPtr      Type = GeoUInt8Property::create();
    GeoUInt32PropertyMTRecPtr     Length = GeoUInt32Property::create();
    GeoPnt3fPropertyMTRecPtr      Pos = GeoPnt3fProperty::create();
    GeoVec3fPropertyMTRecPtr      Norms = GeoVec3fProperty::create();
    GeoUInt32PropertyMTRecPtr     Indices = GeoUInt32Property::create();
    SimpleMaterialMTRecPtr        Mat = SimpleMaterial::create();

    float r0 = hole;
    float r1 = radius();
    int tN = teeth_number;
    float ts = teeth_size;

    //positionen und Normalen
    r1 -= ts*0.5;
    float a[4], s[4], c[4];
    float z = width*0.5;
    float b = z*tan(bevel);
    float bt = ts*cos(bevel);
    float bz = -ts*sin(bevel);
    Vec3d n;
    int iN = 0;
    for(int i=0; i<tN; i++) {
        for (int j=0; j<4; j++) a[j] = 2*Pi*(i+j/6.)/tN;
        for (int j=0; j<4; j++) { c[j] = cos(a[j]); s[j] = sin(a[j]); }

        iN = Pos->size();
        Pos->addValue(Vec3d(c[0]*r0, s[0]*r0, -z)); // 0
        Pos->addValue(Vec3d(c[0]*(r1-b), s[0]*(r1-b), -z)); // 1
        Pos->addValue(Vec3d(c[0]*(r1-b+bt*0.5), s[0]*(r1-b+bt*0.5), -z+bz*0.5)); // 2
        Pos->addValue(Vec3d(c[1]*(r1-b+bt), s[1]*(r1-b+bt), -z+bz)); // 3
        Pos->addValue(Vec3d(c[2]*(r1-b+bt), s[2]*(r1-b+bt), -z+bz)); // 4
        Pos->addValue(Vec3d(c[3]*(r1-b+bt*0.5), s[3]*(r1-b+bt*0.5), -z+bz*0.5)); // 5
        Pos->addValue(Vec3d(c[3]*(r1-b), s[3]*(r1-b), -z)); // 6
        Pos->addValue(Vec3d(c[3]*r0, s[3]*r0, -z)); // 7

        Pos->addValue(Vec3d(c[0]*r0, s[0]*r0, z)); // 8
        Pos->addValue(Vec3d(c[0]*(r1+b), s[0]*(r1+b), z)); // 9
        Pos->addValue(Vec3d(c[0]*(r1+b+bt*0.5), s[0]*(r1+b+bt*0.5), z+bz*0.5)); // 10
        Pos->addValue(Vec3d(c[1]*(r1+b+bt), s[1]*(r1+b+bt), z+bz)); // 11
        Pos->addValue(Vec3d(c[2]*(r1+b+bt), s[2]*(r1+b+bt), z+bz)); // 12
        Pos->addValue(Vec3d(c[3]*(r1+b+bt*0.5), s[3]*(r1+b+bt*0.5), z+bz*0.5)); // 13
        Pos->addValue(Vec3d(c[3]*(r1+b), s[3]*(r1+b), z)); // 14
        Pos->addValue(Vec3d(c[3]*r0, s[3]*r0, z)); // 15

        for (int j=0; j<8; j++) { n = Vec3d(0,0,-1); Norms->addValue(n); }
        for (int j=0; j<8; j++) { n = Vec3d(0,0,1); Norms->addValue(n); }

        Indices->addValue(iN+1);
        Indices->addValue(iN+0); // T1 unten
        Indices->addValue(iN+7);
        Indices->addValue(iN+6);

        Indices->addValue(iN+2);
        Indices->addValue(iN+1); // T2 unten
        Indices->addValue(iN+6);
        Indices->addValue(iN+5);

        Indices->addValue(iN+3);
        Indices->addValue(iN+2); // T3 unten
        Indices->addValue(iN+5);
        Indices->addValue(iN+4);

        Indices->addValue(iN+8); // T1 oben
        Indices->addValue(iN+9);
        Indices->addValue(iN+14);
        Indices->addValue(iN+15);

        Indices->addValue(iN+9); // T2 oben
        Indices->addValue(iN+10);
        Indices->addValue(iN+13);
        Indices->addValue(iN+14);

        Indices->addValue(iN+10); // T3 oben
        Indices->addValue(iN+11);
        Indices->addValue(iN+12);
        Indices->addValue(iN+13);

        Indices->addValue(iN+6);
        Indices->addValue(iN+7); // N unten
        if (i<tN-1) {
            Indices->addValue(iN+16);
            Indices->addValue(iN+17);
        } else { // loop closing quad
            Indices->addValue(0);
            Indices->addValue(1);
        }

        Indices->addValue(iN+15); // N oben
        Indices->addValue(iN+14);
        if (i<tN-1) {
            Indices->addValue(iN+25);
            Indices->addValue(iN+24);
        } else { // loop closing quad
            Indices->addValue(9);
            Indices->addValue(8);
        }
    }

    // sides
    int iNs = Pos->size();
    for(int i=0; i<tN; i++) {
        for (int j=0; j<4; j++) a[j] = 2*Pi*(i+j/6.)/tN;
        for (int j=0; j<4; j++) { c[j] = cos(a[j]); s[j] = sin(a[j]); }

        iN = Pos->size();
        Pos->addValue(Vec3d(c[0]*r0, s[0]*r0, -z)); // 0
        Pos->addValue(Vec3d(c[0]*(r1-b), s[0]*(r1-b), -z)); // 1
        Pos->addValue(Vec3d(c[0]*(r1-b+bt*0.5), s[0]*(r1-b+bt*0.5), -z+bz*0.5)); // 2
        Pos->addValue(Vec3d(c[1]*(r1-b+bt), s[1]*(r1-b+bt), -z+bz)); // 3
        Pos->addValue(Vec3d(c[2]*(r1-b+bt), s[2]*(r1-b+bt), -z+bz)); // 4
        Pos->addValue(Vec3d(c[3]*(r1-b+bt*0.5), s[3]*(r1-b+bt*0.5), -z+bz*0.5)); // 5
        Pos->addValue(Vec3d(c[3]*(r1-b), s[3]*(r1-b), -z)); // 6
        Pos->addValue(Vec3d(c[3]*r0, s[3]*r0, -z)); // 7

        Pos->addValue(Vec3d(c[0]*r0, s[0]*r0, z)); // 8
        Pos->addValue(Vec3d(c[0]*(r1+b), s[0]*(r1+b), z)); // 9
        Pos->addValue(Vec3d(c[0]*(r1+b+bt*0.5), s[0]*(r1+b+bt*0.5), z+bz*0.5)); // 10
        Pos->addValue(Vec3d(c[1]*(r1+b+bt), s[1]*(r1+b+bt), z+bz)); // 11
        Pos->addValue(Vec3d(c[2]*(r1+b+bt), s[2]*(r1+b+bt), z+bz)); // 12
        Pos->addValue(Vec3d(c[3]*(r1+b+bt*0.5), s[3]*(r1+b+bt*0.5), z+bz*0.5)); // 13
        Pos->addValue(Vec3d(c[3]*(r1+b), s[3]*(r1+b), z)); // 14
        Pos->addValue(Vec3d(c[3]*r0, s[3]*r0, z)); // 15

        n = Vec3d(-c[0], -s[0], 0); n.normalize(); Norms->addValue(n);
        n = Vec3d(c[0]+s[0], -c[0]+s[0], 0); n.normalize(); Norms->addValue(n);
        n = Vec3d(s[0], -c[0], 0); n.normalize(); Norms->addValue(n);
        n = Vec3d(c[1]+s[1], -c[1]+s[1], 0); n.normalize(); Norms->addValue(n);
        n = Vec3d(c[2]-s[2], c[2]+s[2], 0); n.normalize(); Norms->addValue(n);
        n = Vec3d(-s[3], c[3], 0); n.normalize(); Norms->addValue(n);
        n = Vec3d(c[3]-s[3], c[3]+s[3], 0); n.normalize(); Norms->addValue(n);
        n = Vec3d(-c[3], -s[3], 0); n.normalize(); Norms->addValue(n);

        n = Vec3d(-c[0], -s[0], 0); n.normalize(); Norms->addValue(n);
        n = Vec3d(c[0]+s[0], -c[0]+s[0], 0); n.normalize(); Norms->addValue(n);
        n = Vec3d(s[0], -c[0], 0); n.normalize(); Norms->addValue(n);
        n = Vec3d(c[1]+s[1], -c[1]+s[1], 0); n.normalize(); Norms->addValue(n);
        n = Vec3d(c[2]-s[2], c[2]+s[2], 0); n.normalize(); Norms->addValue(n);
        n = Vec3d(-s[3], c[3], 0); n.normalize(); Norms->addValue(n);
        n = Vec3d(c[3]-s[3], c[3]+s[3], 0); n.normalize(); Norms->addValue(n);
        n = Vec3d(-c[3], -s[3], 0); n.normalize(); Norms->addValue(n);

        Indices->addValue(iN+0); // B1
        Indices->addValue(iN+8);
        Indices->addValue(iN+15);
        Indices->addValue(iN+7);

        Indices->addValue(iN+7); // B2
        Indices->addValue(iN+15);
        if (i<tN-1) {
            Indices->addValue(iN+24);
            Indices->addValue(iN+16);
        } else { // loop closing quad
            Indices->addValue(iNs+8);
            Indices->addValue(iNs+0);
        }

        Indices->addValue(iN+9);
        Indices->addValue(iN+1); // O1
        Indices->addValue(iN+2);
        Indices->addValue(iN+10);

        Indices->addValue(iN+10);
        Indices->addValue(iN+2); // O2
        Indices->addValue(iN+3);
        Indices->addValue(iN+11);

        Indices->addValue(iN+11);
        Indices->addValue(iN+3); // O3
        Indices->addValue(iN+4);
        Indices->addValue(iN+12);

        Indices->addValue(iN+12);
        Indices->addValue(iN+4); // O4
        Indices->addValue(iN+5);
        Indices->addValue(iN+13);

        Indices->addValue(iN+13);
        Indices->addValue(iN+5); // O5
        Indices->addValue(iN+6);
        Indices->addValue(iN+14);

        Indices->addValue(iN+14);
        Indices->addValue(iN+6); // O6
        if (i<tN-1) {
            Indices->addValue(iN+17);
            Indices->addValue(iN+25);
        } else { // loop closing quad
            Indices->addValue(iNs+1);
            Indices->addValue(iNs+9);
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



float VRGear::radius() { return 0.5*pitch*teeth_number/Pi; }
