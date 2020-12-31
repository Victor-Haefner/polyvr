#ifndef VRMECHANISM_H_INCLUDED
#define VRMECHANISM_H_INCLUDED

#include <vector>
#include "VRMechanismFwd.h"
#include "core/math/OSGMathFwd.h"
#include "core/objects/geometry/VRGeometry.h"

class VRGear;
class VRScrewthread;

OSG_BEGIN_NAMESPACE;
using namespace std;

class MPart;

class VRProfile {
    private:
        vector<Vec2d> pnts;

    public:
        //VRProfile();
        //~VRProfile();

        void add(Vec2d v);
        vector<Vec3d> get(Vec3d n = Vec3d(0,0,1), Vec3d u = Vec3d(0,1,0));
};

struct MChange {
    Vec3d t; // translation
    float l = 0; // translation length
    Vec3d n; // rotation axis
    float a = 0; // rotation angle
    float dx = 0;
    unsigned int time = 0;
    MPart* origin = 0;
    bool doMove = true;

    void flip();
    bool same(MChange c);
    bool isNull();
};

struct MRelation {
    string type;
    MPart* part1 = 0;
    MPart* part2 = 0;

    virtual void translateChange(MChange& change);
};

struct MChainGearRelation : public MRelation {
    int dir = -1;
    MPart* next = 0;
    MPart* prev = 0;
    int segID = 0;

    MChainGearRelation();
    void translateChange(MChange& change);
};

struct MGearGearRelation : public MRelation {
    bool doFlip = true;

    MGearGearRelation();
    void translateChange(MChange& change);
};

struct MObjRelation : public MRelation {

    MObjRelation();
    void translateChange(MChange& change);
};

struct pointPolySegment {
    Vec3d Pseg;
    Vec3d seg;
    float dist2 = 0;
    int ID = 0;
};

class MPart {
    public:
        enum STATE {
            FREE,
            ENGAGED,
            ENGAGING,
            DISENGAGING
        };

        bool resetPhysics = false;
        string type = "part";
        map<MPart*, MRelation*> neighbors;
        vector<MPart*> group;
        VRTransformPtr geo = 0;
        VRTransformPtr trans = 0;
        VRPrimitive* prim = 0;
        MChange change;
        Matrix4d reference;
        unsigned int timestamp = 0;
        STATE state = FREE;

        MPart();
        virtual ~MPart();
        bool changed();

        void apply();
        void setBack();
        MChange getChange();
        bool propagateMovement();
        bool propagateMovement(MChange c, MRelation* r);

        void clearNeighbors();
        void addNeighbor(MPart* p, MRelation* r);
        bool hasNeighbor(MPart* p);
        void computeState();

        void printChange();
        void printNeighbors();

        virtual void setup();
        virtual void computeChange();
        virtual void move();
        virtual void updateNeighbors(vector<MPart*> parts) = 0;

        static MPart* make(VRTransformPtr g, VRTransformPtr t);
};

class MGear : public MPart {
    public:
        Vec3d axis = Vec3d(0,0,-1);
        Vec3d offset = Vec3d(0,0,0);
        Vec3d rAxis;

        MGear();
        ~MGear();

        VRGear* gear();

        void setup();

        void computeChange();
        void move();
        void updateNeighbors(vector<MPart*> parts);
};

class MThread : public MPart {
    public:
        MThread();
        ~MThread();

        VRScrewthread* thread();

        void move();
        void updateNeighbors(vector<MPart*> parts);
};

class MChain : public MPart {
    public:
        enum CSTATE {
            WHOLE,
            BROKEN
        };

        string dirs;
        CSTATE cstate = WHOLE;
        vector<Vec3d> polygon;

        MChain();
        ~MChain();

        VRTransformPtr init();
        void setDirs(string dirs);
        void addDir(char dir);
        void updateGeo();
        vector<pointPolySegment> toPolygon(Vec3d p);

        void move();
        void updateNeighbors(vector<MPart*> parts);
};

class VRMechanism : public VRObject {
    private:
        map<VRTransformPtr, vector<MPart*>> cache;
        vector<MPart*> parts;

        VRAnalyticGeometryPtr geo;

    public:
        VRMechanism();
        ~VRMechanism();
        static VRMechanismPtr create();

        void clear();
        void add(VRTransformPtr part, VRTransformPtr trans = 0);
        void addGear(VRTransformPtr trans, float width, float hole, float pitch, int N_teeth, float teeth_size, float bevel, Vec3d axis, Vec3d offset);
        VRTransformPtr addChain(float w, vector<VRTransformPtr> geos, string dirs);

        void update();
        void updateNeighbors();
        void updateVisuals();
};

OSG_END_NAMESPACE;

#endif // VRMECHANISM_H_INCLUDED
