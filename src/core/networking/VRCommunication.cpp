#include "VRCommunication.h"
#include "core/scene/VRSceneManager.h"


OSG_BEGIN_NAMESPACE
using namespace std;

string VRCommunication::fromInt(int i) {char buf[100]; sprintf(buf,"%d",i); return string(buf); }
string VRCommunication::fromVec3f(Vec3f v) {char buf[100]; sprintf(buf,"%.3g %.3g %.3g", v[0], v[1], v[2]); return string(buf); }

void VRCommunication::parse_cmd() {
    cout << "\nPCMD: " << cmd;

    //parse command id
    cmd_section = atoi(cmd.substr(0, cmd.find(':')).c_str());
    cmd_command = cmd_section%100;
    cmd_section -= cmd_command;
    cmd_section /= 100;

    //parse cmd args
    cmd = cmd.substr(3, cmd.find(';'));
    istringstream iss(cmd);
    vector<string> tmp_args;
    copy(istream_iterator<string>(iss), istream_iterator<string>(), back_inserter<vector<string> >(tmp_args));
    cmd_args.clear();
    for(unsigned int i=0;i<tmp_args.size();i++) {
        cout << "  a " << tmp_args[i];
        cmd_args.push_back(atof(tmp_args[i].c_str()));
    }
    cout << flush;
}

string VRCommunication::process_section() {
    switch (cmd_section) {
        case 0:
            return process_setup_cmd();
            break;
        case 1:
            return process_scene_cmd();
            break;
        case 2:
            return process_object_cmd();
            break;
        case 3:
            return process_navigation_cmd();
            break;
        case 4:
            return process_callbacks_cmd();
            break;
        case 5:
            return process_timeline_cmd();
            break;
        case 9:
            return process_demo_cmd();
            break;
    }
    return fromInt(1);
}

string VRCommunication::process_setup_cmd() {//0
    switch (cmd_command) {
        case 0:
            switch ((int)cmd_args[0]) {
                case 0:
                    VRSceneManager::get()->setStereo((int)cmd_args[1]);
                    break;
                case 1:
                    VRSceneManager::get()->setStereoEyeSeparation(cmd_args[1]);
                    break;
            }

            break;
        case 1:
            break;
        case 2:
            break;
        case 3:
            break;
        case 4:
            break;
        case 5:
            break;
    }
    return fromInt(1);
}

string VRCommunication::process_scene_cmd() {//1
    switch (cmd_command) {
        case 1://get number of scenes
            return fromInt(VRSceneManager::get()->getSceneNum());
            break;
        case 2://get name of scene i
            //return VRSceneManager::get()->getSceneName((int)cmd_args[0]);
            break;
        case 3://set active scene i
            //VRSceneManager::get()->setActiveSceneByID((int)cmd_args[0]);//NOT THREADSAFE!
            break;
        case 4://get active scenes name
            return VRSceneManager::get()->getActiveScene()->getName();
            break;
        case 5://exit
            osgExit(); exit(0);
            break;
    }
    return "1";
}

string VRCommunication::process_object_cmd() {//2
    string tmp_str;
    VRObject* obj = 0;
    VR3DEntity* ent = 0;

    if (cmd_args.size() > 0) {
        obj = VRSceneManager::get()->getActiveScene()->get((int)cmd_args[0]);
        if (obj->getType() == "Transform" or obj->getType() == "Geometry") ent = (VR3DEntity*)obj;
    }

    switch (cmd_command) {
        case 1://get sub graph around object with id x
            tmp_str = obj->getName() + ":";
            tmp_str += obj->getType() + ":";
            if (obj->getParent() != 0) tmp_str += fromInt(obj->getParent()->getID()) + ":";
            else tmp_str += "-1:";
            tmp_str += fromInt(obj->getChildrenCount());
            //for (int i=0; i < obj->getChildrenCount(); i++) { tmp_str += fromInt(obj->getChild(i)->getID()) + ":";}
            return tmp_str;
            break;
        case 2://get active scenes root id
            tmp_str = fromInt(VRSceneManager::get()->getActiveScene()->getRoot()->getID());
            return tmp_str;
            break;
        case 3://get object pose (from, at, up)
            if (ent == 0) { return "-1"; break; }
            tmp_str = fromVec3f(ent->getFrom()) + " ";
            tmp_str += fromVec3f(ent->getAt()) + " ";
            tmp_str += fromVec3f(ent->getUp());
            return tmp_str;
            break;
        case 4://set object from, at, up
            if (ent == 0) break;
            switch ((int)cmd_args[1]) {
                case 0:
                    ent->setFrom(Vec3f(cmd_args[2], cmd_args[3], cmd_args[4]));
                    break;
                case 1:
                    ent->setAt(Vec3f(cmd_args[2], cmd_args[3], cmd_args[4]));
                    break;
                case 2:
                    ent->setUp(Vec3f(cmd_args[2], cmd_args[3], cmd_args[4]));
                    break;
            }
            break;
        case 5://set visibility
            if (obj != 0)
            if((int)cmd_args[1] == 0)
                obj->hide();
            if((int)cmd_args[1] == 1)
                obj->show();
            if((int)cmd_args[1] == 2)
                return fromInt(obj->isVisible());
            break;
        case 6://return the id of the child nb i of obj x
            return fromInt(obj->getChild((int)cmd_args[1])->getID());
            break;
    }
    return "1";
}

string VRCommunication::process_navigation_cmd() {//3
    switch (cmd_command) {
        case 0:
            break;
        case 1:
            break;
        case 2:
            break;
        case 3:
            break;
        case 4:
            break;
        case 5:
            break;
    }
    return fromInt(1);
}

string VRCommunication::process_callbacks_cmd() {//4
    /*VRCallbackManager* cbm = VRCallbackManager::get();
    string tmp_str;
    switch (cmd_command) {
        case 1://get number of signals
            return fromInt(cbm->getSigNum());
            break;
        case 2://get number of callbacks
            return fromInt(cbm->getCBNum());
            break;
        case 3://get number of bonds
            return fromInt(cbm->getBondNum());
            break;
        case 4://get info from signal i
            if ((int)cmd_args[0] == 0)
                tmp_str = cbm->getLabel((int)cmd_args[1]);
            if ((int)cmd_args[0] == 1)
                tmp_str = cbm->getSigLabel((int)cmd_args[1]);
            return tmp_str;
            break;
        case 5://get info from callback i
            if ((int)cmd_args[0] == 0)
                tmp_str = cbm->getLabel((int)cmd_args[1]);
            if ((int)cmd_args[0] == 1)
                tmp_str = cbm->getCbLabel((int)cmd_args[1]);
            return tmp_str;
            break;
        case 6://get info from bond i
            tmp_str = fromInt(cbm->getBondSignal((int)cmd_args[0])) + " " + fromInt(cbm->getBondCallback((int)cmd_args[0]));
            return tmp_str;
            break;
        case 7://add bond
            cbm->bond((int)cmd_args[0], (int)cmd_args[1]);
            break;
        case 8://remove bond
            cbm->divorce((int)cmd_args[0], (int)cmd_args[1]);
            break;
    }*/
    return fromInt(1);
}

string VRCommunication::process_timeline_cmd() {//5
    switch (cmd_command) {
        case 0:
            break;
        case 1:
            break;
        case 2:
            break;
        case 3:
            break;
        case 4:
            break;
        case 5:
            break;
    }
    return fromInt(1);
}

string VRCommunication::process_demo_cmd() {//9
    //VRFunction<vector<float> >* fkt;
    float x,y;
    int w = 0;
    int h = 0;
    switch (cmd_command) {
        case 0:
            /*fkt = (VRFunction<vector<float> >*)VRSceneManager::get()->getActiveScene()->getCallback("SimViDeKont_rad");
            if (fkt == 0) return "1";
            (*fkt)(cmd_args);*/
            break;
        case 98://emulate mouse release
            //cout << "\nP " << cmd_args[0] << "  " << cmd_args[1];
            x = cmd_args[0];
            y = cmd_args[1];
            VRSceneManager::get()->getWindowSize(0,w,h);
            //VRMouse::get()->mouse(1,1,x*w,y*h);
            //VRMouse::get()->glutMotion(x,y);
            break;
        case 99://emulate mouse press
            //cout << "\nP " << cmd_args[0] << "  " << cmd_args[1];
             x = cmd_args[0];
             y = cmd_args[1];
            VRSceneManager::get()->getWindowSize(0,w,h);
            //VRMouse::get()->mouse(1,0,x*w,y*h);
            //VRMouse::get()->glutMotion(x,y);
            break;
        case 20://diehl
            /*fkt = (VRFunction<vector<float> >*)VRSceneManager::get()->getActiveScene()->getCallback("Diehl_cb");
            if (fkt == 0) return "1";
            (*fkt)(cmd_args);*/
            break;
    }
    return fromInt(1);
}

string VRCommunication::processEcoflex(string cmd) {
    cmd = cmd.substr(0, cmd.find('\n'));

    if (cmd.find("GET")!=string::npos) {
        if (cmd.find("AnimDITranIncr")!=string::npos) {

            cmd = cmd.substr(cmd.find('(')+1, cmd.find(')') - cmd.find('(') -1);
            cmd = cmd.substr(cmd.find('(')+1, cmd.find(')') - cmd.find('(') -1);

            cout << "\nIN: " << cmd;

            vector<string> args = vector<string>(4);

            args[0] = cmd.substr(0, cmd.find(','));
            cmd = cmd.substr(cmd.find(',')+1, string::npos);
            args[1] = cmd.substr(0, cmd.find(','));
            cmd = cmd.substr(cmd.find(',')+1, string::npos);
            args[2] = cmd.substr(0, cmd.find(','));
            cmd = cmd.substr(cmd.find(',')+1, string::npos);
            args[3] = cmd.substr(0, cmd.find(','));

            cout << "\nARGS: " << args[0] << "  " << args[1] << "  " << args[2] << "  " << args[3] << flush;
            //VRFunction<vector<string> >* fkt = (VRFunction<vector<string> >*)VRSceneManager::get()->getActiveScene()->getCallback("ecoflex_cb");
            //if (fkt != 0) (*fkt)(args);
        }
    }

    return "200 HTTP/1.1";//http ok
}

string VRCommunication::process_cmd() {
    //check for ecoflex
    if (cmd.find("Java")!=string::npos) { return processEcoflex(cmd); }

    parse_cmd();
    string res = process_section();
    cmd_args.clear();
    return res;
}

void VRCommunication::setCmd(string& c) {
    cmd = c;
    c = process_cmd();//Process incomming commands
}

//main thread does the work, OSG aspects..
void VRCommunication::update() {
    if (cmd != "") {
        cout << "\nupdate cmd, process\n";
        res = process_cmd();//Process incomming commands
        cmd = "";
    }
}

VRCommunication::VRCommunication() {
    handler = new VRFunction<string&>("Communication_setCmd", boost::bind(&VRCommunication::setCmd, this, _1));

    //VRFunction<int>* fkt = new VRFunction<int>(boost::bind(&VRCommunication::update, this));
    //VRSceneManager::get()->addUpdateFkt(fkt);
}

VRCommunication* VRCommunication::get() {
    static VRCommunication* singelton = 0;
    if (singelton == 0) singelton = new VRCommunication();
    return singelton;
}

OSG_END_NAMESPACE;
