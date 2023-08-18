#ifndef VRIMGUIVECTOR_H_INCLUDED
#define VRIMGUIVECTOR_H_INCLUDED

#include "VRImguiInput.h"
#include <string>

using namespace std;

class Im_Vector {
    public:
        string ID;
        string label;
        int flags = 0;

        int Nfields = 3;
        double vX = 0;
        double vY = 0;
        double vZ = 0;

        ImInput iX;
        ImInput iY;
        ImInput iZ;

    public:
        Im_Vector(string ID, string label, int flags = 0);

        void set2(double x, double y);
        void set3(double x, double y, double z);

        bool render(int width);
};

#endif // VRIMGUIVECTOR_H_INCLUDED
