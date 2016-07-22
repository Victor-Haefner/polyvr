#ifndef MAIN_CC_INCLUDED
#define MAIN_CC_INCLUDED

#include <stdlib.h>
#include <string>
#include <iostream>
#include <unistd.h>
#include <math.h>

#include <vrpn/vrpn_Connection.h>
#include <vrpn/vrpn_Button.h>
#include <vrpn/vrpn_Analog.h>
#include <vrpn/vrpn_Tracker.h>

#include <openvr.h>

using namespace std;

class Button : public vrpn_Button_Filter {
    private:
    public:
        Button(string id, vrpn_Connection* c) : vrpn_Button_Filter(id.c_str(), c) {}
        void mainloop() {}
};

class Tracker : public vrpn_Tracker {
    private:
    public:
        Tracker(string id, vrpn_Connection* c) : vrpn_Tracker(id.c_str(), c) {}
        void mainloop() {}
};

class Slider : public vrpn_Analog {
    private:
    public:
        Slider(string id, vrpn_Connection* c) : vrpn_Analog(id.c_str(), c) {}
        void mainloop() {}
};

class Device : public Button, public Slider, public Tracker {
    private:
        struct timeval timestamp;
        bool ready = false;
        vrpn_Connection* connection = 0;
        vr::IVRSystem* HMD = 0;
        vr::TrackedDevicePose_t poses[ vr::k_unMaxTrackedDeviceCount ];

        void processEvent() {
            unsigned int key = 0;
            vr::VREvent_t event;
            while( HMD->PollNextEvent( &event, sizeof( event ) ) ) {
                int id = event.trackedDeviceIndex;
                std::cout << "VREvent_TrackedDevice id " << id << std::endl;
                switch(event.eventType) {
                case vr::VREvent_TrackedDeviceActivated: break;
                case vr::VREvent_TrackedDeviceDeactivated: break;
                case vr::VREvent_ButtonUnpress:
                    key = event.data.status.statusState;
                    buttons[key] = 0;
                    break;
                case vr::VREvent_ButtonPress:
                    key = event.data.status.statusState;
                    buttons[key] = 1;
                    //HMD->ResetSeatedZeroPose();
                        /* key:
                         * 1 - Menu, both are set
                         * 2 - side (squeeze), both state's set
                         * 32 - Touchpad
                         * 33 - trigger
                         */
                    break;
                case vr::VREvent_TrackedDeviceUpdated: break;
                }
            }
        }

        void processController() {
            int k = 0;
            for( vr::TrackedDeviceIndex_t i = 0; i < vr::k_unMaxTrackedDeviceCount; i++ ) {
                if(HMD->GetTrackedDeviceClass( i ) == vr::TrackedDeviceClass_Controller){
                    vr::VRControllerState_t state;
                    if( HMD->GetControllerState( i, &state ) ) {
                        channel[3*k+0] = state.rAxis[0].x; // joystick x
                        channel[3*k+1] = state.rAxis[0].y; // joystick y
                        channel[3*k+2] = state.rAxis[1].x; // trigger
                        k++;
                    }
                }
            }

            cout << "  joystick 1: " << channel[0] << ", " << channel[1] << "  trigger 1: " << channel[2];
            cout << "  joystick 2: " << channel[3] << ", " << channel[4] << "  trigger 1: " << channel[5] << endl;
        }

        void updatePose( int k, const vr::HmdMatrix34_t &m ) {
            pos[0] = m.m[0][3];
            pos[1] = m.m[1][3];
            pos[2] = m.m[2][3];

            float w = sqrt(1+m.m[0][0]+m.m[1][1]+m.m[2][2])/2.0;
            d_quat[0] = (m.m[2][1] - m.m[1][2])/(4*w);
            d_quat[1] = (m.m[0][2] - m.m[2][0])/(4*w);
            d_quat[2] = (m.m[1][0] - m.m[0][1])/(4*w);
            d_quat[3] = w;

            d_sensor = k;
            char msgbuf[1000];
            int  len = vrpn_Tracker::encode_to(msgbuf);

            if (connection->pack_message(len, timestamp, position_m_id, d_sender_id, msgbuf, vrpn_CONNECTION_LOW_LATENCY)) {
                fprintf(stderr,"can't write message: tossing\n");
            }
        }

        void updatePoses() {
            int k = 0;
            vr::VRCompositor()->WaitGetPoses(poses, vr::k_unMaxTrackedDeviceCount, NULL, 0 );
            for ( unsigned int i = 0; i < vr::k_unMaxTrackedDeviceCount; ++i ) {
                if ( poses[i].bPoseIsValid ) {
                    switch (HMD->GetTrackedDeviceClass(i)) {
                        case vr::TrackedDeviceClass_Controller:
                        case vr::TrackedDeviceClass_HMD:
                            updatePose( k, poses[i].mDeviceToAbsoluteTracking );
                            k++;
                            break;
                        case vr::TrackedDeviceClass_Invalid: break;
                        case vr::TrackedDeviceClass_Other: break;
                        case vr::TrackedDeviceClass_TrackingReference:
                            //std::cout << "TrackedDeviceClass_TrackingReference\n"<< ConvertSteamVRMatrixToMatrix4( poses[i].mDeviceToAbsoluteTracking) << std::endl;
                            break;
                        default: break;
                    }
                }
            }
        }

        void fetch_values() {
            if (!ready) return;
            processEvent();
            updatePoses();
            processController();
            vrpn_gettimeofday( &timestamp, NULL );
        }

        void report_changes() {
            Button::timestamp = timestamp;
            Button::report_changes();
            Slider::timestamp = timestamp;
            Slider::report_changes( vrpn_CONNECTION_LOW_LATENCY );
        }

    public:
        Device(string name, int Nb, int Nj, int Nt, vrpn_Connection* c) : Button(name, c), Slider(name, c), Tracker(name, c) {
            Button::num_buttons = Nb;
            Slider::num_channel = Nj;
            Tracker::num_sensors = Nt;
            connection = c;

            for( int i=0; i<Nb; i++) Button::buttons[i] = Button::lastbuttons[i] = 0;
            for( int i=0; i<Nj; i++) Slider::channel[i] = Slider::last[i] = 0;

            try {
                vr::EVRInitError err = vr::VRInitError_None;
                HMD = vr::VR_Init( &err, vr::VRApplication_Scene );
                if (HMD) ready = true;
                if (HMD) cout << "---- Vive ready! ----" << endl;
                else cout << "Vive init failed! " << VR_GetVRInitErrorAsSymbol(err) << endl;
            } catch (exception e) { cout << "Vive exception: " << e.what() << endl; }
        }

        ~Device() {}

        void mainloop() {
            fetch_values();
            report_changes();
            server_mainloop();
        }
};

int main (int argc, char *argv[]) {
    int port = vrpn_DEFAULT_LISTEN_PORT_NO; // default port
    string name = "Vive";
    if (argc > 1) port = atoi(argv[1]);
    if (argc > 2) name = argv[2];
    cout << "start VRPN server on port " << port << " with name " << name << endl;

    //vrpn_Connection* connection = new vrpn_Connection_IP(port);
    vrpn_Connection* connection = vrpn_create_server_connection(port);
    Device* dev = new Device(name,33,6,2,connection);

    while (true) {
        dev->mainloop();
        connection->mainloop(); // Send and receive all messages
        usleep(1000);
    }

    delete dev;
}

#endif // MAIN_CC_INCLUDED
