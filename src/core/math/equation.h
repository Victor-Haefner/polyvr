#ifndef EQUATION_H_INCLUDED
#define EQUATION_H_INCLUDED

#include <OpenSG/OSGConfig.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

class equation {
    private:
        double a = 0; // a x³ +
        double b = 0; // b x² +
        double c = 0; // c x  +
        double d = 0; // d    = 0

    public:
        equation(double a, double b, double c, double d);
        int solve(double& x1, double& x2, double& x3);
};

OSG_END_NAMESPACE;

#endif // EQUATION_H_INCLUDED
