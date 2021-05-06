#ifndef PARTITIONTREE_H_INCLUDED_COLL
#define PARTITIONTREE_H_INCLUDED_COLL

#include <stdlib.h>
#include <vector>
#include <OpenSG/OSGConfig.h>
#include "core/math/OSGMathFwd.h"
#include "core/math/VRMathFwd.h"
#include "boundingbox.h"

using namespace std;

OSG_BEGIN_NAMESPACE

class PartitiontreeNode {
    protected:
        float resolution = 0.1;
        float size = 10;
        int level = 0;

        Vec3d center;

        vector<void*> data;
        vector<Vec3d> points;

        bool sphere_box_intersect(Vec3d Ps, Vec3d Pb, float Rs, float Sb);
        bool box_box_intersect(Vec3d min, Vec3d max, Vec3d Bpos, float Sb);

    public:
        PartitiontreeNode(float resolution, float size = 10, int level = 0);
        virtual ~PartitiontreeNode();

        float getSize();
        float getResolution();
        Vec3d getCenter();

        void setResolution(float res);

        void remData(void* data);

        vector<void*> getData();
        vector<Vec3d> getPoints();

        int dataSize();
        void* getData(int i);
        Vec3d getPoint(int i);

        template<class T>
        void delContent() {
            for (void* o : data) delete (T*)o;
            points.clear();
            data.clear();
        }

        string toString(int indent = 0);
};

class Partitiontree : public std::enable_shared_from_this<Partitiontree> {
    protected:
        float resolution = 0.1;
        float firstSize = 10;

    public:
        string name;

    public:
        Partitiontree(float resolution, float size = 10, string name = "");
        virtual ~Partitiontree();
        PartitiontreePtr ptr();
};

OSG_END_NAMESPACE

#endif // PARTITIONTREE_H_INCLUDED
