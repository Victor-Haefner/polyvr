#ifndef VRMECHANISM_H_INCLUDED
#define VRMECHANISM_H_INCLUDED

#include <vector>
#include "core/utils/Thread.h"
#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGMatrix.h>
#include "VRMechanismFwd.h"
#include "core/math/OSGMathFwd.h"
#include "core/objects/object/VRObject.h"

class VRGear;
class VRScrewThread;

OSG_BEGIN_NAMESPACE;
using namespace std;

class MPart;
class MMotor;

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
    unsigned int substep = 0;
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
    void translateChange(MChange& change) override;
};

struct MGearThreadRelation : public MRelation {
    bool doFlip = true;

    MGearThreadRelation();
    void translateChange(MChange& change) override;
};

struct MGearGearRelation : public MRelation {
    bool doFlip = true;

    MGearGearRelation();
    void translateChange(MChange& change) override;
};

struct MObjRelation : public MRelation {

    MObjRelation();
    void translateChange(MChange& change) override;
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
        bool didMove = false;
        string type = "part";
        map<MPart*, MRelation*> neighbors;
        map<MPart*, MRelation*> forcedNeighbors;
        vector<MPart*> group;
        VRTransformPtr geo = 0;
        VRTransformPtr trans = 0;
        shared_ptr<VRPrimitive> prim = 0;
        MChange change;
        MChange cumulativeChange;
        MChange lastChange;
        Matrix4d reference;
        Matrix4d referenceT;
        PosePtr transform;
        unsigned int timestamp = 0;
        STATE state = FREE;

        MPart();
        virtual ~MPart();
        bool changed();

        void updateTransform();
        void apply();
        void setBack();
        MChange getChange();
        bool propagateMovement();
        bool propagateMovement(MChange c, MRelation* r);

        void clearNeighbors();
        void addNeighbor(MPart* p, MRelation* r);
        void addCoaxialNeighbor(MPart* p);
        bool hasNeighbor(MPart* p);
        void computeState();

        void printChange();
        void printNeighbors();

        virtual void setup();
        virtual void computeChange();
        virtual void drivenChange(MMotor* motor, int step, double dt) = 0;
        virtual void move();
        virtual void updateNeighbors(vector<MPart*> parts) = 0;

        static MPart* make(VRTransformPtr g, VRTransformPtr t);
};

class MGear : public MPart {
    public:
        Vec3d axis = Vec3d(0,0,-1);
        Vec3d offset = Vec3d(0,0,0);
        Vec3d rAxis;
        Vec3d rOffset;

        MGear();
        ~MGear();

        shared_ptr<VRGear> gear();

        void setup() override;

        void computeChange() override;
        void drivenChange(MMotor* motor, int step, double dt) override;
        void move() override;
        void updateNeighbors(vector<MPart*> parts) override;
};

class MThread : public MPart {
    public:
        Vec3d axis = Vec3d(0,0,-1);
        Vec3d rAxis;

        MThread();
        ~MThread();

        shared_ptr<VRScrewThread> thread();

        void setup() override;

        void computeChange() override;
        void drivenChange(MMotor* motor, int step, double dt) override;

        void move() override;
        void updateNeighbors(vector<MPart*> parts) override;
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
        bool needsUpdate = false;

        MChain();
        ~MChain();

        VRTransformPtr init();
        void setDirs(string dirs);
        void addDir(char dir);
        void updateGeo();
        vector<pointPolySegment> toPolygon(Vec3d p);

        void move() override;
        void drivenChange(MMotor* motor, int step, double dt) override;
        void updateNeighbors(vector<MPart*> parts) override;
};

class MMotor {
    public:
        string name;
        int dof = 5; // z
        VRTransformPtr driven;
        float speed = 1.0;
};

class VRMechanism : public VRObject {
    private:
        map<VRTransformPtr, vector<MPart*>> cache;
        vector<MPart*> parts;
        vector<MPart*> changed_parts;
        map<string, MMotor*> motors;

        VRAnalyticGeometryPtr mviz;

        bool doRun = true;
        bool doThread = true;
        ::Thread* simThread = 0;
        VRTimerPtr simTime;
        int substep = 0;

        void updateThread();

    public:
        VRMechanism();
        ~VRMechanism();
        static VRMechanismPtr create();

        void clear();
        void add(VRTransformPtr part, VRTransformPtr trans = 0);
        void addGear(VRTransformPtr trans, float width, float hole, float pitch, int N_teeth, float teeth_size, float bevel, Vec3d axis, Vec3d offset);
        VRTransformPtr addChain(float w, vector<VRTransformPtr> geos, string dirs);
        void addCoaxialConstraint(VRTransformPtr part1, VRTransformPtr part2);

        void addMotor(string name, VRTransformPtr driven, float speed = 0.01, int dof = 5);
        void setMotorSpeed(string name, float speed);

        int getNParts();
        double getLastChange(VRTransformPtr part);

        void update(bool fromThread = false);
        void updateNeighbors();
        void updateVisuals();
};

OSG_END_NAMESPACE;

#endif // VRMECHANISM_H_INCLUDED
