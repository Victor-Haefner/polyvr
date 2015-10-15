#include "VRPrimitive.h"
#include <math.h>
#include <OpenSG/OSGSimpleGeometry.h>
#include <OpenSG/OSGGeoProperties.h>
#include <OpenSG/OSGSimpleMaterial.h>

int VRPrimitive::getNParams() { return N; }
string VRPrimitive::getType() { return type; }
void VRPrimitive::fromString(string s) { stringstream ss(s); fromStream(ss); }
string VRPrimitive::toString() { stringstream ss; toStream(ss); return ss.str(); }
VRPrimitive* VRPrimitive::make(string p) {
    if (p == "Plane") return new VRPlane();
    if (p == "Box") return new VRBox();
    if (p == "Sphere") return new VRSphere();
    if (p == "Torus") return new VRTorus();
    if (p == "Teapot") return new VRTeapot();
    if (p == "Cylinder") return new VRCylinder();
    if (p == "Cone") return new VRCone();
    if (p == "Arrow") return new VRArrow();
    if (p == "Gear") return new VRGear();
    if (p == "Thread") return new VRThread();
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

        params["Cone"].push_back("Height");
        params["Cone"].push_back("Radius");
        params["Cone"].push_back("Sides");
        params["Cone"].push_back("Do bottom");
        params["Cone"].push_back("Do sides");

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

        params["Gear"].push_back("Width");
        params["Gear"].push_back("Hole");
        params["Gear"].push_back("Pitch");
        params["Gear"].push_back("Number of teeth");
        params["Gear"].push_back("Teeth size");
        params["Gear"].push_back("Bevel");

        params["Thread"].push_back("Length");
        params["Thread"].push_back("Radius");
        params["Thread"].push_back("Pitch");
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
VRArrow::VRArrow() { N = 4; type = "Arrow"; }
VRThread::VRThread() { N = 4; type = "Thread"; }
VRGear::VRGear() { N = 6; type = "Gear"; }

void VRPlane::fromStream(stringstream& ss) { ss >> width >> height >> Nx >> Ny; }
void VRBox::fromStream(stringstream& ss) { ss >> width >> height >> depth >> Nx >> Ny >> Nz; }
void VRSphere::fromStream(stringstream& ss) { ss >> radius >> iterations; }
void VRTorus::fromStream(stringstream& ss) { ss >> inner_radius >> outer_radius >> Nsegments >> Nrings; }
void VRTeapot::fromStream(stringstream& ss) { ss >> iterations >> scale; }
void VRCylinder::fromStream(stringstream& ss) { ss >> height >> radius >> Nsides >> doTop >> doBottom >> doSides; }
void VRCone::fromStream(stringstream& ss) { ss >> height >> radius >> Nsides >> doBottom >> doSides; }
void VRArrow::fromStream(stringstream& ss) { ss >> height >> width >> trunc >> hat; }
void VRThread::fromStream(stringstream& ss) { ss >> length >> radius >> pitch >> Nsegments; }
void VRGear::fromStream(stringstream& ss) { ss >> width >> hole >> pitch >> teeth_number >> teeth_size >> bevel; }

void VRPlane::toStream(stringstream& ss) { ss << width << " " << height << " " << Nx << " " << Ny; }
void VRBox::toStream(stringstream& ss) { ss << width << " " << height << " " << depth << " " << Nx << " " << Ny << " " << Nz; }
void VRSphere::toStream(stringstream& ss) { ss << radius << " " << iterations; }
void VRTorus::toStream(stringstream& ss) { ss << inner_radius << " " << outer_radius << " " << Nsegments << " " << Nrings; }
void VRTeapot::toStream(stringstream& ss) { ss << iterations << " " << scale; }
void VRCylinder::toStream(stringstream& ss) { ss << height << " " << radius << " " << Nsides << " " << doTop << " " << doBottom << " " << doSides; }
void VRCone::toStream(stringstream& ss) { ss << height << " " << radius << " " << Nsides << " " << doBottom << " " << doSides; }
void VRArrow::toStream(stringstream& ss) { ss << height << " " << width << " " << trunc << " " << hat; }
void VRThread::toStream(stringstream& ss) { ss << length << " " << radius << " " << pitch << " " << Nsegments; }
void VRGear::toStream(stringstream& ss) { ss << width << " " << hole << " " << pitch << " " << teeth_number << " " << teeth_size << " " << bevel; }

OSG::GeometryRecPtr VRPlane::make() { return OSG::makePlaneGeo(width, height, Nx, Ny); }
OSG::GeometryRecPtr VRBox::make() { return OSG::makeBoxGeo(width, height, depth, Nx, Ny, Nz); }
OSG::GeometryRecPtr VRSphere::make() { return OSG::makeSphereGeo(iterations, radius); }
OSG::GeometryRecPtr VRTorus::make() { return OSG::makeTorusGeo(inner_radius, outer_radius, Nsegments, Nrings); }
OSG::GeometryRecPtr VRTeapot::make() { return OSG::makeTeapotGeo(iterations, scale); }
OSG::GeometryRecPtr VRCylinder::make() { return OSG::makeCylinderGeo(height, radius, Nsides, doSides, doTop, doBottom); }
OSG::GeometryRecPtr VRCone::make() { return OSG::makeConeGeo(height, radius, Nsides, doSides, doBottom); }
OSG::GeometryRecPtr VRArrow::make() {
    OSG::GeoUInt8PropertyRecPtr      Type = OSG::GeoUInt8Property::create();
    OSG::GeoUInt32PropertyRecPtr     Length = OSG::GeoUInt32Property::create();
    OSG::GeoPnt3fPropertyRecPtr      Pos = OSG::GeoPnt3fProperty::create();
    OSG::GeoVec3fPropertyRecPtr      Norms = OSG::GeoVec3fProperty::create();
    OSG::GeoUInt32PropertyRecPtr     Indices = OSG::GeoUInt32Property::create();
    OSG::SimpleMaterialRecPtr        Mat = OSG::SimpleMaterial::create();

    Pos->addValue(OSG::Vec3f(0,0,0));
    Pos->addValue(OSG::Vec3f(-width*0.5,0,hat));
    Pos->addValue(OSG::Vec3f(width*0.5,0,hat));
    Pos->addValue(OSG::Vec3f(-trunc*0.5,0,hat));
    Pos->addValue(OSG::Vec3f(trunc*0.5,0,hat));
    Pos->addValue(OSG::Vec3f(-trunc*0.5,0,height));
    Pos->addValue(OSG::Vec3f(trunc*0.5,0,height));
    for (int i=0; i<7; i++) Norms->addValue(OSG::Vec3f(0,1,0));
    Indices->addValue(0); Indices->addValue(1); Indices->addValue(2);
    Indices->addValue(3); Indices->addValue(5); Indices->addValue(4);
    Indices->addValue(4); Indices->addValue(5); Indices->addValue(6);

    Type->addValue(GL_TRIANGLES);
    Length->addValue(Indices->size()); // for each tooth 4 quads

    Mat->setDiffuse(OSG::Color3f(0.8,0.8,0.6));
    Mat->setAmbient(OSG::Color3f(0.4, 0.4, 0.2));
    Mat->setSpecular(OSG::Color3f(0.1, 0.1, 0.1));

    OSG::GeometryRecPtr geo = OSG::Geometry::create();
    geo->setTypes(Type);
    geo->setLengths(Length);
    geo->setIndices(Indices);
    geo->setPositions(Pos);
    geo->setNormals(Norms);
    geo->setMaterial(Mat);

    return geo;
}
OSG::GeometryRecPtr VRThread::make() {
    OSG::GeoUInt8PropertyRecPtr      Type = OSG::GeoUInt8Property::create();
    OSG::GeoUInt32PropertyRecPtr     Length = OSG::GeoUInt32Property::create();
    OSG::GeoPnt3fPropertyRecPtr      Pos = OSG::GeoPnt3fProperty::create();
    OSG::GeoVec3fPropertyRecPtr      Norms = OSG::GeoVec3fProperty::create();
    OSG::GeoUInt32PropertyRecPtr     Indices = OSG::GeoUInt32Property::create();
    OSG::SimpleMaterialRecPtr        Mat = OSG::SimpleMaterial::create();

    int rN = Nsegments;

    //positionen und Normalen
    OSG::Vec3f n;
    int iN = 0;
    int tN = round(length/pitch);
    float r1 = radius;
    float r2 = radius+0.5*pitch/tan(M_PI/6);
    for(int i=0; i<tN; i++) {
        iN = Pos->size();
        for (int j=0; j<rN; j++) {
            float sa = sin(j*2*M_PI/rN);
            float ca = cos(j*2*M_PI/rN);
            float o = j*pitch/rN;

            Pos->addValue(OSG::Vec3f(r1*ca ,r1*sa ,o+i*pitch));
            Pos->addValue(OSG::Vec3f(r2*ca ,r2*sa ,o+(i+0.5)*pitch));
            Norms->addValue(OSG::Vec3f(ca,sa,0));
            Norms->addValue(OSG::Vec3f(ca,sa,0));

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

    Mat->setDiffuse(OSG::Color3f(0.8,0.8,0.6));
    Mat->setAmbient(OSG::Color3f(0.4, 0.4, 0.2));
    Mat->setSpecular(OSG::Color3f(0.1, 0.1, 0.1));

    OSG::GeometryRecPtr geo = OSG::Geometry::create();
    geo->setTypes(Type);
    geo->setLengths(Length);
    geo->setIndices(Indices);
    geo->setPositions(Pos);
    geo->setNormals(Norms);
    geo->setMaterial(Mat);

    return geo;
}
OSG::GeometryRecPtr VRGear::make() {
    OSG::GeoUInt8PropertyRecPtr      Type = OSG::GeoUInt8Property::create();
    OSG::GeoUInt32PropertyRecPtr     Length = OSG::GeoUInt32Property::create();
    OSG::GeoPnt3fPropertyRecPtr      Pos = OSG::GeoPnt3fProperty::create();
    OSG::GeoVec3fPropertyRecPtr      Norms = OSG::GeoVec3fProperty::create();
    OSG::GeoUInt32PropertyRecPtr     Indices = OSG::GeoUInt32Property::create();
    OSG::SimpleMaterialRecPtr        Mat = OSG::SimpleMaterial::create();

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
    OSG::Vec3f n;
    int iN = 0;
    for(int i=0; i<tN; i++) {
        for (int j=0; j<4; j++) a[j] = 2*M_PI*(i+j/6.)/tN;
        for (int j=0; j<4; j++) { c[j] = cos(a[j]); s[j] = sin(a[j]); }

        iN = Pos->size();
        Pos->addValue(OSG::Vec3f(c[0]*r0, s[0]*r0, -z)); // 0
        Pos->addValue(OSG::Vec3f(c[0]*(r1-b), s[0]*(r1-b), -z)); // 1
        Pos->addValue(OSG::Vec3f(c[0]*(r1-b+bt*0.5), s[0]*(r1-b+bt*0.5), -z+bz*0.5)); // 2
        Pos->addValue(OSG::Vec3f(c[1]*(r1-b+bt), s[1]*(r1-b+bt), -z+bz)); // 3
        Pos->addValue(OSG::Vec3f(c[2]*(r1-b+bt), s[2]*(r1-b+bt), -z+bz)); // 4
        Pos->addValue(OSG::Vec3f(c[3]*(r1-b+bt*0.5), s[3]*(r1-b+bt*0.5), -z+bz*0.5)); // 5
        Pos->addValue(OSG::Vec3f(c[3]*(r1-b), s[3]*(r1-b), -z)); // 6
        Pos->addValue(OSG::Vec3f(c[3]*r0, s[3]*r0, -z)); // 7

        Pos->addValue(OSG::Vec3f(c[0]*r0, s[0]*r0, z)); // 8
        Pos->addValue(OSG::Vec3f(c[0]*(r1+b), s[0]*(r1+b), z)); // 9
        Pos->addValue(OSG::Vec3f(c[0]*(r1+b+bt*0.5), s[0]*(r1+b+bt*0.5), z+bz*0.5)); // 10
        Pos->addValue(OSG::Vec3f(c[1]*(r1+b+bt), s[1]*(r1+b+bt), z+bz)); // 11
        Pos->addValue(OSG::Vec3f(c[2]*(r1+b+bt), s[2]*(r1+b+bt), z+bz)); // 12
        Pos->addValue(OSG::Vec3f(c[3]*(r1+b+bt*0.5), s[3]*(r1+b+bt*0.5), z+bz*0.5)); // 13
        Pos->addValue(OSG::Vec3f(c[3]*(r1+b), s[3]*(r1+b), z)); // 14
        Pos->addValue(OSG::Vec3f(c[3]*r0, s[3]*r0, z)); // 15

        for (int j=0; j<8; j++) { n = OSG::Vec3f(0,0,-1); Norms->addValue(n); }
        for (int j=0; j<8; j++) { n = OSG::Vec3f(0,0,1); Norms->addValue(n); }

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
        for (int j=0; j<4; j++) a[j] = 2*M_PI*(i+j/6.)/tN;
        for (int j=0; j<4; j++) { c[j] = cos(a[j]); s[j] = sin(a[j]); }

        iN = Pos->size();
        Pos->addValue(OSG::Vec3f(c[0]*r0, s[0]*r0, -z)); // 0
        Pos->addValue(OSG::Vec3f(c[0]*(r1-b), s[0]*(r1-b), -z)); // 1
        Pos->addValue(OSG::Vec3f(c[0]*(r1-b+bt*0.5), s[0]*(r1-b+bt*0.5), -z+bz*0.5)); // 2
        Pos->addValue(OSG::Vec3f(c[1]*(r1-b+bt), s[1]*(r1-b+bt), -z+bz)); // 3
        Pos->addValue(OSG::Vec3f(c[2]*(r1-b+bt), s[2]*(r1-b+bt), -z+bz)); // 4
        Pos->addValue(OSG::Vec3f(c[3]*(r1-b+bt*0.5), s[3]*(r1-b+bt*0.5), -z+bz*0.5)); // 5
        Pos->addValue(OSG::Vec3f(c[3]*(r1-b), s[3]*(r1-b), -z)); // 6
        Pos->addValue(OSG::Vec3f(c[3]*r0, s[3]*r0, -z)); // 7

        Pos->addValue(OSG::Vec3f(c[0]*r0, s[0]*r0, z)); // 8
        Pos->addValue(OSG::Vec3f(c[0]*(r1+b), s[0]*(r1+b), z)); // 9
        Pos->addValue(OSG::Vec3f(c[0]*(r1+b+bt*0.5), s[0]*(r1+b+bt*0.5), z+bz*0.5)); // 10
        Pos->addValue(OSG::Vec3f(c[1]*(r1+b+bt), s[1]*(r1+b+bt), z+bz)); // 11
        Pos->addValue(OSG::Vec3f(c[2]*(r1+b+bt), s[2]*(r1+b+bt), z+bz)); // 12
        Pos->addValue(OSG::Vec3f(c[3]*(r1+b+bt*0.5), s[3]*(r1+b+bt*0.5), z+bz*0.5)); // 13
        Pos->addValue(OSG::Vec3f(c[3]*(r1+b), s[3]*(r1+b), z)); // 14
        Pos->addValue(OSG::Vec3f(c[3]*r0, s[3]*r0, z)); // 15

        n = OSG::Vec3f(-c[0], -s[0], 0); n.normalize(); Norms->addValue(n);
        n = OSG::Vec3f(c[0]+s[0], -c[0]+s[0], 0); n.normalize(); Norms->addValue(n);
        n = OSG::Vec3f(s[0], -c[0], 0); n.normalize(); Norms->addValue(n);
        n = OSG::Vec3f(c[1]+s[1], -c[1]+s[1], 0); n.normalize(); Norms->addValue(n);
        n = OSG::Vec3f(c[2]-s[2], c[2]+s[2], 0); n.normalize(); Norms->addValue(n);
        n = OSG::Vec3f(-s[3], c[3], 0); n.normalize(); Norms->addValue(n);
        n = OSG::Vec3f(c[3]-s[3], c[3]+s[3], 0); n.normalize(); Norms->addValue(n);
        n = OSG::Vec3f(-c[3], -s[3], 0); n.normalize(); Norms->addValue(n);

        n = OSG::Vec3f(-c[0], -s[0], 0); n.normalize(); Norms->addValue(n);
        n = OSG::Vec3f(c[0]+s[0], -c[0]+s[0], 0); n.normalize(); Norms->addValue(n);
        n = OSG::Vec3f(s[0], -c[0], 0); n.normalize(); Norms->addValue(n);
        n = OSG::Vec3f(c[1]+s[1], -c[1]+s[1], 0); n.normalize(); Norms->addValue(n);
        n = OSG::Vec3f(c[2]-s[2], c[2]+s[2], 0); n.normalize(); Norms->addValue(n);
        n = OSG::Vec3f(-s[3], c[3], 0); n.normalize(); Norms->addValue(n);
        n = OSG::Vec3f(c[3]-s[3], c[3]+s[3], 0); n.normalize(); Norms->addValue(n);
        n = OSG::Vec3f(-c[3], -s[3], 0); n.normalize(); Norms->addValue(n);

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

    Mat->setDiffuse(OSG::Color3f(0.8,0.8,0.6));
    Mat->setAmbient(OSG::Color3f(0.4, 0.4, 0.2));
    Mat->setSpecular(OSG::Color3f(0.1, 0.1, 0.1));

    OSG::GeometryRecPtr geo = OSG::Geometry::create();
    geo->setTypes(Type);
    geo->setLengths(Length);
    geo->setIndices(Indices);
    geo->setPositions(Pos);
    geo->setNormals(Norms);
    geo->setMaterial(Mat);

    return geo;
}



float VRGear::radius() { return 0.5*pitch*teeth_number/M_PI; }
