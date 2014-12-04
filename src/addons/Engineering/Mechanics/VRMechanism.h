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
};

class MPart {
    public:
        enum STATE {
            FREE,
            ENGAGED,
            ENGAGING,
            DISENGAGING
        };

        map<MPart*, int> neighbors;
        vector<MPart*> group;
        VRGeometry* geo = 0;
        VRTransform* trans = 0;
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
        bool propagateMovement();
        bool propagateMovement(MChange c, int flip);

        void clearNeighbors();
        void addNeighbor(MPart* p, int flip);
        bool hasNeighbor(MPart* p);
        void computeState();

        void printChange();
        void printNeighbors();

        virtual void computeChange();
        virtual void move();
        virtual void updateNeighbors(vector<MPart*> parts) = 0;

        static MPart* make(VRGeometry* g, VRTransform* t);
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

        VRGeometry* init();
        void setDirs(string dirs);
        void addDir(char dir);
        void updateGeo();
        void toPolygon(Vec3f p, Vec3f& ps, Vec3f& sd);

        void move();
        void updateNeighbors(vector<MPart*> parts);
};

class VRMechanism {
    private:
        map<VRGeometry*, MPart*> cache;
        vector<MPart*> parts;

    public:
        VRMechanism();
        ~VRMechanism();

        void add(VRGeometry* part, VRTransform* trans = 0);
        void clear();
        void update();
        VRGeometry* addChain(float w, vector<VRGeometry*> geos, string dirs);
};

OSG_END_NAMESPACE;

#endif // VRMECHANISM_H_INCLUDED
