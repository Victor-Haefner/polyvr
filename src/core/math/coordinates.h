#ifndef COORDINATES_H_INCLUDED
#define COORDINATES_H_INCLUDED

#include <OpenSG/OSGMatrix.h>

class coords {
    public:
        static void YtoZ(OSG::Matrix& m);
        static void ZtoY(OSG::Matrix& m);
};

#endif // COORDINATES_H_INCLUDED
