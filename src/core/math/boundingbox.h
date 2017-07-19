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

        void setCenter(const Vec3d& t);
        void move(const Vec3d& t);
        void scale(float s);

        bool isInside(Vec3d p) const;
        bool intersectedBy(Line l);

        Vec3d getRandomPoint();
};

OSG_END_NAMESPACE;

#endif // BOUNDINGBOX_H_INCLUDED
