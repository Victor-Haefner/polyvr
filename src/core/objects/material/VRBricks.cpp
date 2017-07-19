#include "VRBricks.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

VRBricks::VRBricks() {}

void VRBricks::apply(Color4f* data, Vec3i dim, float amount, Color4f c1, Color4f c2) { // not tested
	int d = amount;
	int I = max(1,dim[0]);
	int J = max(1,dim[1]);
	int K = max(1,dim[2]);
	int I4 = I*0.25;
	int J4 = J*0.25;
	int K4 = K*0.25;

    for (int k=0; k<K; k++) {
        for (int j=0; j<J; j++) {
            for (int i=0; i<I; i++) {
                bool yPlane = (j > J4-d && j < J4+d) || (j > 3*J4-d && j < 3*J4+d);
                bool inX = (j >= J4+d && j <= 3*J4-d) && (i > 3*I4-d && i < 3*I4+d);
                bool inZ = (j >= J4+d && j <= 3*J4-d) && (k > K4-d && k < K4+d);
                bool outX = (j <= J4-d || j >= 3*J4+d) && (i > I4-d && i < I4+d);
                bool outZ = (j <= J4-d || j >= 3*J4+d) && (k > 3*K4-d && k < 3*K4+d);

                if (yPlane || inX || inZ || outX || outZ) {
                    int l = k*J*I + j*I + i;
                    Color4f c = data[l];
                    c[0] *= c1[0]; c[1] *= c1[1]; c[2] *= c1[2]; c[3] *= c1[3];
                    data[l] = c;
                }
            }
        }
    }
}

void VRBricks::apply(Color3f* data, Vec3i dim, float amount, Color3f c1, Color3f c2) {
	int d = amount;
	int I = max(1,dim[0]);
	int J = max(1,dim[1]);
	int K = max(1,dim[2]);
	int I4 = I*0.25;
	int J4 = J*0.25;
	int K4 = K*0.25;

    for (int k=0; k<K; k++) {
        for (int j=0; j<J; j++) {
            for (int i=0; i<I; i++) {
                bool yPlane = (j > J4-d && j < J4+d) || (j > 3*J4-d && j < 3*J4+d);
                bool inX = (j >= J4+d && j <= 3*J4-d) && (i > 3*I4-d && i < 3*I4+d);
                bool inZ = (j >= J4+d && j <= 3*J4-d) && (k > K4-d && k < K4+d);
                bool outX = (j <= J4-d || j >= 3*J4+d) && (i > I4-d && i < I4+d);
                bool outZ = (j <= J4-d || j >= 3*J4+d) && (k > 3*K4-d && k < 3*K4+d);

                if (yPlane || inX || inZ || outX || outZ) {
                    int l = k*J*I + j*I + i;
                    Color3f c = data[l];
                    c[0] *= c1[0]; c[1] *= c1[1]; c[2] *= c1[2];
                    data[l] = c;
                }
            }
        }
    }
}

OSG_END_NAMESPACE;
