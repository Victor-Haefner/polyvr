#ifndef EQUATION_H_INCLUDED
#define EQUATION_H_INCLUDED

#include <OpenSG/OSGConfig.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

class equation {
    private:
        float a = 0; // a x³ +
        float b = 0; // b x² +
        float c = 0; // c x  +
        float d = 0; // d    = 0

    public:
        equation(float a, float b, float c, float d);
        int solve(float& x1, float& x2, float& x3);
};

OSG_END_NAMESPACE;

#endif // EQUATION_H_INCLUDED
