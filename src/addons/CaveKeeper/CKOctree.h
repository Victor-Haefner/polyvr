#ifndef OCTREE_H_INCLUDED
#define OCTREE_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGLine.h>
#include <string.h>

#include "core/utils/VRFunction.h"

OSG_BEGIN_NAMESPACE
using namespace std;

class Octree;

class CKOctree {
    public:
        struct light {
            Vec3f pos;
            light(Vec3f p) : pos(p) {}
        };

        struct element {
            int ID = 0;
            element* parent = 0;
            element* children[8];
            int childN = 0;
            CKOctree* tree = 0;
            bool octIsEmpty[8]; //flag to see if octant is empty || solid earth

            Vec3f pos;
            Vec3i otpos;
            int size;
            int _size;
            int octant = 0;
            bool leaf = 0;

            int type;
            int chunk;

            Vec4f vertexLight[6];

            element(CKOctree* tree, Vec3f p, Vec3i otp, int s);
            ~element();

            void add(element* e);

            int getOctant(Vec3i p);
            int getOctant(Vec3f p);

            Vec3i getOctantVec(int o);

            bool inside(Vec3f f);
            bool inside(Vec3i f);

            void print(string indent = "");

            void updateLightning(light* l);
            void updateLightning(vector<void*> lights);
        };

    private:
		Octree* lightTree = 0;
        map<int,element*> elements;
        element* root = 0;
        int N = 0;
        Vec3f hitPoint;
        Vec3f hitNormal;
        element* hitElement = 0;

        int getMax(Vec3i i);

        void print(element* e, string indent = "");

        int signof(float f);

    public:
        CKOctree();

        element* add(Vec3i _p);
        void rem(element* e);
        void addAround(element* e);
        light* addLight(Vec3f p);

        //check if there is a cube at pos
        bool isLeaf(Vec3f p);

        //check if space || solid at pos
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
        element* getElement(int i);

        Vec3f getHitPoint();
        Vec3f getHitNormal();
        element* getHitElement();

        void print();
};

OSG_END_NAMESPACE

#endif // OCTREE_H_INCLUDED
