#include "equation.h"
#include <math.h>

using namespace OSG;

equation::equation(float a, float b, float c, float d) : a(a), b(b), c(c), d(d) {;}

int equation::solve(float& x1, float& x2, float& x3) {
    if (a == 0) {
        if (b == 0) { // linear equation
            if (c == 0) return 0;
            x1 = -d/c;
            return 1;
        }

        // quadratic
        double delta = (c*c-4*b*d);
        double inv_2a = 1.0/2/b;
        if (delta >= 0) {
            double root = sqrt(delta);
            x1 = (-b-root)*inv_2a;
            x2 = (-b+root)*inv_2a;
            return 2;
        } else return 0;
    }

    // cubic (TODO: reuse from driving simulator asphalt shader)
    return 0;
}
