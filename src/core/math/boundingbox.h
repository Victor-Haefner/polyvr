#ifndef BOUNDINGBOX_H_INCLUDED
#define BOUNDINGBOX_H_INCLUDED

#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGLine.h>
#include "core/objects/VRObjectFwd.h"
#include "core/math/VRMathFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class Boundingbox {
    private:
        Vec3d bb1, bb2;
        bool cleared = true;

    public:
        Boundingbox();
        static BoundingboxPtr create();

        void clear();
        bool empty() const;

        void update(const Vec3d& v);
        void updateFromPoints(const vector<Vec3d>& v);
        void updateFromGeometry(VRGeometryPtr g);

        Vec3d min() const;
        Vec3d max() const;
        Vec3d center() const;
        Vec3d size() const;
        float radius() const;
        float volume() const;

        void setCenter(const Vec3d& t);
        void move(const Vec3d& t);
        void scale(float s);
        void inflate(float D);

        bool isInside(Vec3d p) const;
        bool intersectedBy(Line l);
        bool intersect(BoundingboxPtr bb);

        void clamp(Vec3d& p) const;

        Vec3d getRandomPoint();

        VRGeometryPtr asGeometry();

        // python proxies
        bool py_empty();
        void py_update(Vec3d v);
        void py_updateFromPoints(vector<Vec3d> v);
        Vec3d py_min();
        Vec3d py_max();
        Vec3d py_center();
        Vec3d py_size();
        float py_radius();
        float py_volume();
        void py_setCenter(Vec3d t);
        void py_move(Vec3d t);
        bool py_isInside(Vec3d p);
        void py_clamp(Vec3d p);
};

OSG_END_NAMESPACE;

#endif // BOUNDINGBOX_H_INCLUDED
