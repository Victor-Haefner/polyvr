#ifndef VRMECHANISM_H_INCLUDED
#define VRMECHANISM_H_INCLUDED

#include <vector>
#include <OpenSG/OSGVector.h>
#include "core/objects/geometry/VRGeometry.h"

class VRGear;

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

class MPart {
    public:
        vector<MPart*> neighbors;
        vector<MPart*> group;
        VRGeometry* geo;
        VRTransform* trans;
        VRPrimitive* prim;
        Matrix reference;
        unsigned int timestamp;

        MPart();
        virtual ~MPart();
        bool changed();

        void computeChange();
        void apply();
        void setBack();
        bool propagateMovement();

        void clearNeighbors();
        void addNeighbor(MPart* p);

        virtual void move(float dx);
        virtual void updateNeighbors(vector<MPart*> parts) = 0;

        static MPart* make(VRGeometry* g, VRTransform* t);
};

class MGear : public MPart {
    public:
        MGear();
        ~MGear();

        void move(float dx);
        void updateNeighbors(vector<MPart*> parts);
};

class MThread : public MPart {
    public:
        MThread();
        ~MThread();

        void move(float dx);
        void updateNeighbors(vector<MPart*> parts);
};

class MChain : public MPart {
    public:
        MChain();
        ~MChain();

        void move(float dx);
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
        void addChain(float w, vector<VRGeometry*> geos);
};

OSG_END_NAMESPACE;

#endif // VRMECHANISM_H_INCLUDED
