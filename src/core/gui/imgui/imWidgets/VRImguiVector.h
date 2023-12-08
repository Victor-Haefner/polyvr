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
        double vW = 0;

        ImInput iX;
        ImInput iY;
        ImInput iZ;
        ImInput iW;

    public:
        Im_Vector(string ID, string label, int flags = 0);

        void set2(double x, double y);
        void set3(double x, double y, double z);
        void set4(double x, double y, double z, double w);

        void set2(string s);
        void set3(string s);
        void set4(string s);

        bool render(int width);
        void signal(string sig);
};

#endif // VRIMGUIVECTOR_H_INCLUDED
