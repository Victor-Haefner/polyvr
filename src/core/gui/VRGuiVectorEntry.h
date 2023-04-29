#ifndef VRGUIVECTORENTRY_H_INCLUDED
#define VRGUIVECTORENTRY_H_INCLUDED

#include <string>
#include <functional>
#include "core/math/OSGMathFwd.h"

using namespace std;

class VRGuiVectorEntry {
    private:
        string label;

    public:
        VRGuiVectorEntry();

        void init(string placeholder, string label, function<void(OSG::Vec3d&)> sig);
        void init2D(string placeholder, string label, function<void(OSG::Vec2d&)> sig);

        void set(OSG::Vec3d v);
        void set(OSG::Vec2d v);

        void setFontColor(OSG::Vec3d c);
};

#endif // VRGUIVECTORENTRY_H_INCLUDED
