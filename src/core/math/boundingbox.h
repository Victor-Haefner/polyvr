#ifndef BOUNDINGBOX_H_INCLUDED
#define BOUNDINGBOX_H_INCLUDED

#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGLine.h>
#include "core/objects/VRObjectFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class boundingbox {
    private:
        Vec3f bb1, bb2;
        bool cleared = true;

    public:
        boundingbox();
        void clear();
        bool empty();

        void update(Vec3f v);
        void update(VRGeometryPtr g);

        Vec3f min();
        Vec3f max();
        Vec3f center();
        Vec3f size();
        float radius();

        void move(const Vec3f& t);

        bool intersectedBy(Line l);
};

OSG_END_NAMESPACE;

#endif // BOUNDINGBOX_H_INCLUDED
