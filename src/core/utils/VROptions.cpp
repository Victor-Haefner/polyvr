#include "VROptions.h"

#include <boost/program_options/parsers.hpp>
#include <iostream>
#include <fstream>
#include <iomanip>

using namespace std;
namespace bpo = boost::program_options;

VROptions::VROptions() : desc("Configuration ") {
    cout << " setup command line options" << endl;
    desc.add_options() ("help", "show possible options");

    addOption<bool>(true, "dofailcheck", "do a fail check of the last startup of PolyVR, may halt startup");
    addOption<bool>(false, "standalone", "start without UI, only GL canvas");
    addOption<bool>(false, "maximized", "start with maximized GL window");
    addOption<bool>(false, "fullscreen", "start with fullscreen GL window");
    addOption<bool>(false, "ignoreHMD", "ignore HMD if present");
    addOption<bool>(false, "active_stereo", "use active_stereo or not");
    addOption<bool>(false, "nogtk", "start without UI, even without GL canvas, you should add a glut window in the setup file");
    addOption<string>("", "application", "specify an application file to load at startup");
    addOption<string>("", "decryption", "pass information to decrypt a secured application, \"key:YOURKEY\"");
    addOption<string>("", "setup", "specify the hardware setup file to load, ommiting this will load the last setup");

    cout << endl;
}

VROptions::~VROptions() {
    cout << "VROptions::~VROptions" << endl;
}

void VROptions::operator= (VROptions v) {;}

VROptions* VROptions::get() {
    static VROptions* singleton = new VROptions();
    return singleton;
}

void VROptions::parse(int _argc, char** _argv) {
    argc = _argc;
    argv = _argv;

    cout << " parse command line options ( " << argc << " parameter(s) )" << endl;
    for (int i=0; i<argc; i++) cout << "  " << i+1 << ") '" << _argv[i] << "'" << endl;

    try {
        bpo::positional_options_description p;
        p.add("application", -1);

        //bpo::store(bpo::parse_command_line(argc, argv, desc), vm);
        bpo::store(bpo::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);
        bpo::notify(vm);
        if (vm.count("help")) { cout << desc << endl; exit(1); }
    } catch(exception& e) {
        cout << "VROptions::parse exception: " << e.what() << endl;
    } catch(...) {
        cout << "VROptions::parse unknown exception" << endl;
        cout << desc << endl;
    }

    cout << endl;
}
