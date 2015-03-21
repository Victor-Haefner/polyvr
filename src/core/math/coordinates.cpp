#include "coordinates.h"

using namespace OSG;
using namespace std;

void coords::YtoZ(Matrix& m) {
    swap(m[0][1], m[0][2]);
    swap(m[1][1], m[1][2]);
    swap(m[2][1], m[2][2]);
    swap(m[3][1], m[3][2]);

    m[0][2] *= -1;
    m[1][2] *= -1;
    m[2][2] *= -1;
    m[3][2] *= -1;
}

void coords::ZtoY(Matrix& m) {
    swap(m[0][1], m[0][2]);
    swap(m[1][1], m[1][2]);
    swap(m[2][1], m[2][2]);
    swap(m[3][1], m[3][2]);

    m[0][1] *= -1;
    m[1][1] *= -1;
    m[2][1] *= -1;
    m[3][1] *= -1;
}
