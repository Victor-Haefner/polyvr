#ifndef VRDRIVER_H_INCLUDED
#define VRDRIVER_H_INCLUDED

#include <OpenSG/OSGVector.h>
#include "addons/Bullet/VRPhysicsFwd.h"
#include "core/math/VRMathFwd.h"
#include "core/utils/VRFunctionFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRDriver {
    private:
        CarDynamicsPtr car;
        bool active = false;
        pathPtr p_path;
        pathPtr v_path;
        VRUpdateCbPtr updatePtr;

        void update();

    public:
        VRDriver();
        ~VRDriver();
        static VRDriverPtr create();

        //void setTask(); // TODO
        void setCar( CarDynamicsPtr car );
        void followPath(pathPtr p, pathPtr v);
        void stop();
        bool isDriving();
};

/* TODOs

- add statemachine to utils or math folder
- add VRTask to addons/training, with a state machine

*/

OSG_END_NAMESPACE;

#endif // VRDRIVER_H_INCLUDED
