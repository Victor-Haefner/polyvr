#include "virtuoseAPI.h"
#include "../../../networking/VRSharedMemory.h"

#include <iostream>
#include <unistd.h>

using namespace std;

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>

using namespace boost::interprocess;

struct Vec9 { float data[9] = {0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f}; };
struct Vec7 { float data[7] = {0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,1.0f}; };
struct Vec6 { float data[6] = {0.0f,0.0f,0.0f,0.0f,0.0f,0.0f}; };
struct Vec3 { float data[3] = {0.0f,0.0f,0.0f}; };

template <typename T>
void print(const T& t, int N) {
    cout << " trans: ";
    for (int i=0; i<N; i++) cout << t.data[i] << " ";
    cout << endl;
}

class Device {
    private:
        VRSharedMemory interface;
        VirtContext vc;

        Vec7 identity;
        Vec6 forces;
        Vec7 position;

        bool attached = false;
        bool run = true;
        bool shifting = true;

        void updateVirtuoseState() {
            interface.lock();
            run = interface.getObject<bool>("run", false);
            virtGetAvatarPosition(vc, position.data);
            interface.setObject<Vec7>("position", position);

            int _shifting = 0;
            int power = 0;
            virtGetPowerOn(vc, &power);
            virtIsInShiftPosition(vc, &_shifting);
            shifting = bool(_shifting || !power);
            interface.setObject<bool>("shifting", shifting);
            interface.unlock();
        }

        void applyVirtuoseForces() {
            /*if (interface.hasObject<Vec6>("targetForces")) {
                auto forces = interface.getObject<Vec6>("targetForces");
                //print(forces, 6);
                virtAddForce(vc, forces.data);
            }*/
        }

        void applyVirtuosePose() {
            //cout << "\nattached " << attached << " hasTargetSpeed " << interface.hasObject<Vec6>("targetSpeed") << " targetPosition " << interface.hasObject<Vec7>("targetPosition");
            if (attached && interface.hasObject<Vec6>("targetSpeed") && interface.hasObject<Vec7>("targetPosition")) {
                auto targetSpeed = interface.getObject<Vec6>("targetSpeed");
                auto targetPosition = interface.getObject<Vec7>("targetPosition");
                float tmpPos[7];
                virtGetPosition(vc, tmpPos);
                for(int i = 0; i < 7 ; i++) tmpPos[i] = targetPosition.data[i] - tmpPos[i];
                virtSetPosition(vc, targetPosition.data);

                float tmpSp[6];
                virtGetSpeed(vc, tmpSp);
                for(int i = 0; i < 6 ; i++) tmpSp[i] = targetSpeed.data[i] - tmpSp[i];
                virtSetSpeed(vc, targetSpeed.data);

                float Lp = sqrt( tmpPos[0]*tmpPos[0]+tmpPos[1]*tmpPos[1]+tmpPos[2]*tmpPos[2] );
                float Ls = sqrt( tmpSp[0]*tmpSp[0]+tmpSp[1]*tmpSp[1]+tmpSp[2]*tmpSp[2] );
                float Lt = sqrt( tmpSp[3]*tmpSp[3]+tmpSp[4]*tmpSp[4]+tmpSp[5]*tmpSp[5] );
                bool doPhysUpdate = ( Lp < 0.1 && Ls < 0.5 && Lt < 0.5 );
                interface.setObject<bool>("doPhysUpdate", doPhysUpdate);
            }
        }

        void transmitForces() {
            virtGetForce(vc, forces.data);
            interface.setObject<Vec6>("forces", forces);
            //cout << "device: "; print(forces, 6);
        }

        void handleCommands() {
            //cout << "\n has doAttach " << interface.hasObject<bool>("doAttach") << endl;
            if (interface.getObject<bool>("doAttach", false)) {
                cout << " --- virtuose attach!! ---\n";
                interface.setObject<bool>("doAttach", false);
                auto mass = interface.getObject<float>("mass");
                auto inertia = interface.getObject<Vec9>("inertia");
                virtAttachVO(vc, mass, inertia.data);
                attached = true;
            }

            //float position[7] = {0.0,0.0,0.0,0.0,0.0,0.0,1.0};
            //float speed[6] = {0.0,0.0,0.0,0.0,0.0,0.0};
            //virtSetPosition(vc, position);
            //virtSetSpeed(vc, speed);
        }

    public:
        Device() : interface("virtuose", true, false) {
            interface.addObject<Vec7>("position");
            interface.setObject<bool>("run", true);


            vc = virtOpen("172.22.151.200");
            if (!vc) {
                cout << "starting virtuose deamon failed, no connection to device!\n";
                return;
                //cout << " error code 2: " << virtGetErrorMessage(2) << endl;
            }


            virtSetIndexingMode(vc, INDEXING_ALL_FORCE_FEEDBACK_INHIBITION);
            virtSetSpeedFactor(vc, 1);
            virtSetForceFactor(vc, 1);
            virtSetTimeStep(vc, 0.02);
            virtSetBaseFrame(vc, identity.data);
            virtSetObservationFrame(vc, identity.data);
            virtSetCommandType(vc, COMMAND_TYPE_VIRTMECH);
            virtSetDebugFlags(vc, DEBUG_SERVO|DEBUG_LOOP);
            virtEnableForceFeedback(vc, 1);
            virtSetPowerOn(vc, 1);

            float K[] = { 10, 10, 10, 1000, 1000, 1000 };
            float B[] = { 0.1, 0.1, 0.1, 10, 10, 10 };
            virtAttachQSVO(vc, K, B);
        }

        void start() {
            do {
                updateVirtuoseState();
                if (!shifting) {
                    applyVirtuoseForces();
                    applyVirtuosePose();
                    transmitForces();
                    handleCommands();
                }
                usleep(2000);
            } while (run);
        }

        ~Device() {
            virtSetPowerOn(vc, 0);
            virtDetachVO(vc);
            virtStopLoop(vc);
            virtClose(vc);
        }
};

int main() {
    Device dev;
    dev.start();
    return 0;
}
