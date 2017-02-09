#ifndef VRMECHANISM_H_INCLUDED
#define VRMECHANISM_H_INCLUDED

#include <vector>
#include <OpenSG/OSGVector.h>
#include "core/objects/geometry/VRGeometry.h"

class VRGear;
class VRThread;

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRProfile {
    private:
        vector<Vec2f> pnts;

    public:
        //VRProfile();
        //~VRProfile();

        void add(Vec2f v);
        vector<Vec3f> get(Vec3f n = Vec3f(0,0,1), Vec3f u = Vec3f(0,1,0));
};

struct MChange {
    Vec3f t; // translation
    float l = 0; // translation length
    Vec3f n; // rotation axis
    float a = 0; // rotation angle
    float dx = 0;
    uint time = 0;

    void flip();
    bool same(MChange c);
    bool isNull();
};

class MPart;
struct MRelation {
    MPart* part1 = 0;
    MPart* part2 = 0;

    virtual void translateChange(MChange& change);
};

struct MChainGearRelation : public MRelation {
    int dir = -1;
    MPart* next = 0;
    MPart* prev = 0;
    int segID = 0;

    void translateChange(MChange& change);
};

struct MGearGearRelation : public MRelation {
    void translateChange(MChange& change);
};

struct pointPolySegment {
    Vec3f Pseg;
    Vec3f seg;
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

        map<MPart*, MRelation*> neighbors;
        vector<MPart*> group;
        VRGeometryPtr geo = 0;
        VRTransformPtr trans = 0;
        VRPrimitive* prim = 0;
        MChange change;
        Matrix reference;
        uint timestamp = 0;
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

        virtual void computeChange();
        virtual void move();
        virtual void updateNeighbors(vector<MPart*> parts) = 0;

        static MPart* make(VRGeometryPtr g, VRTransformPtr t);
};

class MGear : public MPart {
    public:
        MGear();
        ~MGear();

        VRGear* gear();

        void computeChange();
        void move();
        void updateNeighbors(vector<MPart*> parts);
};

class MThread : public MPart {
    public:
        MThread();
        ~MThread();

        VRThread* thread();

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
        vector<Vec3f> polygon;

        MChain();
        ~MChain();

        VRGeometryPtr init();
        void setDirs(string dirs);
        void addDir(char dir);
        void updateGeo();
        vector<pointPolySegment> toPolygon(Vec3f p);

        void move();
        void updateNeighbors(vector<MPart*> parts);
};

class VRMechanism {
    private:
        map<VRGeometryPtr, MPart*> cache;
        vector<MPart*> parts;

    public:
        VRMechanism();
        ~VRMechanism();
        static shared_ptr<VRMechanism> create();

        void add(VRGeometryPtr part, VRTransformPtr trans = 0);
        void clear();
        void update();
        VRGeometryPtr addChain(float w, vector<VRGeometryPtr> geos, string dirs);
};

OSG_END_NAMESPACE;

#endif // VRMECHANISM_H_INCLUDED
