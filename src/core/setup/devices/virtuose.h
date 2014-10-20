#ifndef VIRTUOSE_H_INCLUDED
#define VIRTUOSE_H_INCLUDED

#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGMatrix.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

class virtuose {
    private:
        void* vc = 0; // virtuose context

    public:
        virtuose();
        ~virtuose();

        bool connected();
        void connect(string IP);
        void disconnect();

        void setSimulationScales(float translation, float forces);
        void applyForce(Vec3f force, Vec3f torque);
        Matrix getPose();
};

OSG_END_NAMESPACE;

#endif // VIRTUOSE_H_INCLUDED
