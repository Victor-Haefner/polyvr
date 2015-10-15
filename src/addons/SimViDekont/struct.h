#include "core/objects/geometry/VRGeometry.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

 typedef struct vec {

            float x[3];

            vec(float a, float b, float c) {
                x[0] = a;
                x[1] = b;
                x[2] = c;
            }

            vec() {
                x[0] = 0;
                x[1] = 0;
                x[2] = 0;
            }

        } vec;

   struct frame {
            uint id;
            vector<vec>* pos;
            vector<int>* ind;
            vector<vec>* norm;
            vector<float>* stress;
            vector<float>* strain;
            vector<Vec2f>* texs;

            VRGeometryPtr geo;

            frame(int N) :
                        pos(new vector<vec>(N)),
                        ind(new vector<int>(N)),
                        norm(new vector<vec>(N)),
                        stress(new vector<float>(N)),
                        strain(new vector<float>(N)),
                        texs(new vector<Vec2f>(N))
                        {}

            frame() : pos(0), ind(0), norm(0), stress(0), texs(0) {}

            ~frame() {
                if (pos != 0) delete pos;
                if (ind != 0) delete ind;
                if (norm != 0) delete norm;
                if (stress != 0) delete stress;
                if (texs != 0) delete texs;
            }
        };
OSG_END_NAMESPACE;
