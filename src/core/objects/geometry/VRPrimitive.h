#ifndef VRPRIMITIVE_H_INCLUDED
#define VRPRIMITIVE_H_INCLUDED

#include <string>
#include <OpenSG/OSGSField.h>

namespace OSG{ class Geometry; OSG_GEN_CONTAINERPTR(Geometry); }

using namespace std;

struct VRPrimitive {
    virtual OSG::GeometryMTRecPtr make() = 0;

    int getNParams();
    string getType();
    void fromString(string s);
    string toString();

    static VRPrimitive* create(string p);
    static vector<string> getTypes();
    static vector<string> getTypeParameter(string type);

    protected:
        int N;
        string type;

        virtual void fromStream(stringstream& ss) = 0;
        virtual void toStream(stringstream& ss) = 0;
};

struct VRPlane : public VRPrimitive {
    float width = 1;
    float height = 1;
    float Nx = 1;
    float Ny = 1;

    VRPlane();
    void fromStream(stringstream& ss);
    void toStream(stringstream& ss);
    OSG::GeometryMTRecPtr make();
};

struct VRBox : public VRPrimitive {
    float width = 1;
    float height = 1;
    float depth = 1;
    float Nx = 1;
    float Ny = 1;
    float Nz = 1;

    VRBox();
    void fromStream(stringstream& ss);
    void toStream(stringstream& ss);
    OSG::GeometryMTRecPtr make();
};

struct VRSphere : public VRPrimitive {
    float radius = 1;
    int iterations = 2;

    VRSphere();
    void fromStream(stringstream& ss);
    void toStream(stringstream& ss);
    OSG::GeometryMTRecPtr make();
};

struct VRTorus : public VRPrimitive {
    float inner_radius = 0.5;
    float outer_radius = 1;
    int Nsegments = 16;
    int Nrings = 16;

    VRTorus();
    void fromStream(stringstream& ss);
    void toStream(stringstream& ss);
    OSG::GeometryMTRecPtr make();
};

struct VRTeapot : public VRPrimitive {
    float scale = 1;
    int iterations = 2;

    VRTeapot();
    void fromStream(stringstream& ss);
    void toStream(stringstream& ss);
    OSG::GeometryMTRecPtr make();
};

struct VRCone : public VRPrimitive {
    float height = 1;
    float radius = 1;
    int Nsides = 16;
    bool doSides = true;
    bool doBottom = true;

    VRCone();
    void fromStream(stringstream& ss);
    void toStream(stringstream& ss);
    OSG::GeometryMTRecPtr make();
};

struct VRCylinder : public VRPrimitive {
    float height = 1;
    float radius = 1;
    int Nsides = 16;
    bool doSides = true;
    bool doTop = true;
    bool doBottom = true;

    VRCylinder();
    void fromStream(stringstream& ss);
    void toStream(stringstream& ss);
    OSG::GeometryMTRecPtr make();
};

struct VRPill : public VRPrimitive {
    float height = 1;
    float radius = 1;
    int Nsides = 16;
    bool doSides = true;
    bool doTop = true;
    bool doBottom = true;

    VRPill();
    void fromStream(stringstream& ss);
    void toStream(stringstream& ss);
    OSG::GeometryMTRecPtr make();
};

struct VRArrow : public VRPrimitive {
    float height = 1;
    float width = 1;
    float trunc = 0.5;
    float hat = 0.2;
    float thickness = 0.2;

    VRArrow();
    void fromStream(stringstream& ss);
    void toStream(stringstream& ss);
    OSG::GeometryMTRecPtr make();
};

struct VRScrewThread : public VRPrimitive {
    float length = 1;
    float radius = 0.5;
    float pitch = 0.1;
    float Nsegments = 16;

    VRScrewThread();
    void fromStream(stringstream& ss);
    void toStream(stringstream& ss);
    OSG::GeometryMTRecPtr make();
};

struct VRGear : public VRPrimitive {
    float width = 0.1;
    float hole = 0.3;
    float pitch = 0.1;
    float teeth_size = 0.05;
    float bevel = 0;
    int teeth_number = 32;
    float offset = 0;

    VRGear();
    VRGear(float width, float hole, float pitch, int N_teeth, float teeth_size, float bevel);
    void fromStream(stringstream& ss);
    void toStream(stringstream& ss);
    OSG::GeometryMTRecPtr make();
    float radius();
};

#endif // VRPRIMITIVE_H_INCLUDED
