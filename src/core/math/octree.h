#ifndef OCTREE_H_INCLUDED
#define OCTREE_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGLine.h>
#include <string.h>

#include "core/utils/VRFunction.h"

OSG_BEGIN_NAMESPACE
using namespace std;

class octree {
    public:
        struct element {
            int ID;
            element* parent;
            element* children[8];
            int childN;
            bool octIsEmpty[8]; //flag to see if octant is empty or solid earth

            Vec3f pos;
            Vec3i otpos;
            int size;
            int _size;
            int octant;
            bool leaf;

            int type;
            int chunk;

            Vec4f vertexLight[6];

            element(Vec3f p, Vec3i otp, int s);

            void add(element* e);

            int getOctant(Vec3i p);
            int getOctant(Vec3f p);

            Vec3i getOctantVec(int o);

            bool inside(Vec3f f);
            bool inside(Vec3i f);

            void print(string indent = "");
        };

    private:
        element* root;
        int N;
        Vec3f hitPoint;
        Vec3f hitNormal;
        element* hitElement;

        int getMax(Vec3i i);

        void print(element* e, string indent = "");

        int signof(float f);

    public:
        octree();

        void add(Vec3i _p);
        void rem(element* e);
        void addAround(element* e);


        //check if there is a cube at pos
        bool isLeaf(Vec3f p);

        //check if space or solid at pos
        bool isEmpty(Vec3f p);
        void setEmpty(Vec3i p);

        //get the smallest element at that position
        element* get(Vec3f p, element* e = 0);
        //ray cast
        element* get(Line ray, element* e = 0, string indent = "");

        vector<element*> getAround(Vec3f pos, float r);

        void traverse(VRFunction<element*>* cb);
        void traverse(element* e, VRFunction<element*>* cb);

        element* getRoot();

        Vec3f getHitPoint();
        Vec3f getHitNormal();
        element* getHitElement();

        void print();
};

OSG_END_NAMESPACE

#endif // OCTREE_H_INCLUDED
