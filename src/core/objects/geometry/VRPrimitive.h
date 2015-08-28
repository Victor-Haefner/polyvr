#ifndef VRPRIMITIVE_H_INCLUDED
#define VRPRIMITIVE_H_INCLUDED

#include <string>
#include <OpenSG/OSGFieldContainerFields.h>

namespace OSG{ class Geometry; OSG_GEN_CONTAINERPTR(Geometry); }

using namespace std;

struct VRPrimitive {
    virtual OSG::GeometryRecPtr make() = 0;

    int getNParams();
    string getType();
    void fromString(string s);
    string toString();

    static VRPrimitive* make(string p);
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
    OSG::GeometryRecPtr make();
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
    OSG::GeometryRecPtr make();
};

struct VRSphere : public VRPrimitive {
    float radius = 1;
    int iterations = 2;

    VRSphere();
    void fromStream(stringstream& ss);
    void toStream(stringstream& ss);
    OSG::GeometryRecPtr make();
};

struct VRTorus : public VRPrimitive {
    float inner_radius = 0.5;
    float outer_radius = 1;
    int Nsegments = 16;
    int Nrings = 16;

    VRTorus();
    void fromStream(stringstream& ss);
    void toStream(stringstream& ss);
    OSG::GeometryRecPtr make();
};

struct VRTeapot : public VRPrimitive {
    float scale = 1;
    int iterations = 2;

    VRTeapot();
    void fromStream(stringstream& ss);
    void toStream(stringstream& ss);
    OSG::GeometryRecPtr make();
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
    OSG::GeometryRecPtr make();
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
    OSG::GeometryRecPtr make();
};

struct VRArrow : public VRPrimitive {
    float height = 1;
    float width = 1;
    float trunc = 0.5;
    float hat = 0.2;

    VRArrow();
    void fromStream(stringstream& ss);
    void toStream(stringstream& ss);
    OSG::GeometryRecPtr make();
};

struct VRThread : public VRPrimitive {
    float length = 1;
    float radius = 0.5;
    float pitch = 0.1;
    float Nsegments = 16;

    VRThread();
    void fromStream(stringstream& ss);
    void toStream(stringstream& ss);
    OSG::GeometryRecPtr make();
};

struct VRGear : public VRPrimitive {
    float width = 0.1;
    float hole = 0.3;
    float pitch = 0.1;
    float teeth_size = 0.05;
    float bevel = 0;
    int teeth_number = 32;

    VRGear();
    void fromStream(stringstream& ss);
    void toStream(stringstream& ss);
    OSG::GeometryRecPtr make();
    float radius();
};

#endif // VRPRIMITIVE_H_INCLUDED
