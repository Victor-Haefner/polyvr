#ifndef COORDINATES_H_INCLUDED
#define COORDINATES_H_INCLUDED

#include <OpenSG/OSGMatrix.h>

class coords {
    public:
        static void YtoZ(OSG::Matrix4d& m);
        static void ZtoY(OSG::Matrix4d& m);
};

#endif // COORDINATES_H_INCLUDED
