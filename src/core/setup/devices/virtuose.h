#ifndef VIRTUOSE_H_INCLUDED
#define VIRTUOSE_H_INCLUDED
#include <virtuose/virtuoseAPI.h>

#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGMatrix.h>
#include <OpenSG/OSGGLUT.h>


#include <list>
#include "core/objects/VRTransform.h"
#include "core/objects/geometry/VRPhysics.h"


OSG_BEGIN_NAMESPACE;
using namespace std;

class virtuose {
    private:
        void* vc = 0; // virtuose context
        bool isAttached = false;
        VRTransform* attached = 0;
        //haptic timesteps
        float timestep = 0.0f;
        //polyvr time
        float timeLastFrame = glutGet(GLUT_ELAPSED_TIME);
        VirtCommandType commandType  = COMMAND_TYPE_NONE;
        float gripperPosition;
        float gripperSpeed;
        float globalforce[6] = {0.0,0.0,0.0,0.0,0.0,0.0};


    public:
        virtuose();
        ~virtuose();

        bool connected();
        void connect(string IP);
        void disconnect();

        void setSimulationScales(float translation, float forces);


        Matrix getPose();

        /**
        Applies given Force/Torque on the haptic
        **/
        void applyForce(Vec3f force, Vec3f torque);
        /** parses position/rotation data of given VRPhysics into the given float[7] array**/
        void fillPosition(VRPhysics* p, float *to);
        /** parses speed data of given VRPhysics into the given float[6] array**/
        void fillSpeed(VRPhysics* p, float *to);
        /** parses given btMatrix3x3 into the given float[9] array**/
        void Matrix3ToArray(btMatrix3x3 m, float *to);
        /**connect a physicalized Object to this virtuose and push it in the same direction the virtuose moves.**/
        void attachTransform(VRTransform* trans);
        /** detach the previously attached Transform**/
        void detachTransform();
        /** update function of the Virtuose, has to be called each frame**/
        void updateVirtMech();
        /** 1 means button pressed, 0 means released**/
        Vec3i getButtonStates();
};

OSG_END_NAMESPACE;

#endif // VIRTUOSE_H_INCLUDED
