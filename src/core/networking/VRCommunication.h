#ifndef VRCOMMUNICATION_H_INCLUDED
#define VRCOMMUNICATION_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGVector.h>
#include <string>
#include "core/utils/VRFunction.h"

OSG_BEGIN_NAMESPACE
using namespace std;

class VRCommunication {
    private:
        string fromInt(int i);
        string fromVec3f(Vec3f v);

        string cmd;
        int cmd_section, cmd_command;
        vector<float> cmd_args;
        string res;

        void parse_cmd();

        string process_section();
        string process_setup_cmd();
        string process_scene_cmd();
        string process_object_cmd();
        string process_navigation_cmd();
        string process_callbacks_cmd();
        string process_timeline_cmd();
        string process_demo_cmd();
        string processEcoflex(string cmd);
        string process_cmd();

        void setCmd(string& c);

        //main thread does the work, OSG aspects..
        void update();

        VRCommunication();

    public:
        VRFunction<string&>* handler;

        static VRCommunication* get();
};

OSG_END_NAMESPACE;

#endif // VRCOMMUNICATION_H_INCLUDED
