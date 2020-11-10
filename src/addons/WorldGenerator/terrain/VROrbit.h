#ifndef VRORBIT_H_INCLUDED
#define VRORBIT_H_INCLUDED

#include <OpenSG/OSGVector.h>
#include "core/objects/VRObjectFwd.h"
#include "addons/WorldGenerator/VRWorldGeneratorFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VROrbit {
    private:
        struct Kepler {
            double a0; double e0; double I0; double l0; double w0; double O0;
            double da; double de; double dI; double dl; double dw; double dO;
        };

        Kepler params;
        VRObjectPtr trail;
        VRObjectPtr target;
        VRObjectPtr referential;

        double toRad(double deg);

		double computeEccentricAnomaly(double M, double e);

    public:
        VROrbit();
        ~VROrbit();

        static VROrbitPtr create();

        void setReferential(VRObjectPtr obj);
        void setTarget(VRObjectPtr obj);
        void setTrail(VRObjectPtr obj);
        VRObjectPtr getReferential();
        VRObjectPtr getTarget();
        VRObjectPtr getTrail();

		void fromCircle(Vec3d plane, double radius, double speed);
		void fromKepler(vector<double> params);

		Vec3d computeCoords(double Teph);
		double getPeriod();
};

OSG_END_NAMESPACE;

#endif // VRORBIT_H_INCLUDED
