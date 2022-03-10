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

template<class T>
class PartitiontreeNode {
    protected:
        float resolution = 0.1;
        float size = 10;
        int level = 0;

        Vec3d center;

        vector<T> data;
        vector<Vec3d> points;

        bool sphere_box_intersect(Vec3d Ps, Vec3d Pb, float Rs, float Sb);
        bool box_box_intersect(Vec3d min, Vec3d max, Vec3d Bpos, float Sb);

    public:
        PartitiontreeNode(float r, float s = 10, int l = 0) : resolution(r), size(s), level(l) { ; }
        virtual ~PartitiontreeNode() {};

        float getSize() { return size; }
        float getResolution() { return resolution; }
        Vec3d getCenter() { return center; }

        void setResolution(float res) { resolution = res; }

        void remData(const T& d) {
            data.erase(std::remove(data.begin(), data.end(), d), data.end());
        }

        vector<T>& getData() { return data; }
        vector<Vec3d>& getPoints() { return points; }

        size_t dataSize() { return data.size(); }
        T& getData(size_t i) { return data[i]; }
        Vec3d getPoint(size_t i) { return points[i]; }

        void clearContent() {
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
