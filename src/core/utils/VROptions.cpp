#include "VROptions.h"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <map>
#include <vector>
#include <string>

using namespace std;

VROptions::VROptions() {
    cout << " setup command line options" << endl;

    addOption<bool>(true, "dofailcheck", "do a fail check of the last startup of PolyVR, may halt startup");
    addOption<bool>(false, "standalone", "start without UI, only GL canvas");
    addOption<bool>(false, "maximized", "start with maximized GL window");
    addOption<bool>(false, "fullscreen", "start with fullscreen GL window");
    addOption<bool>(false, "ignoreHMD", "ignore HMD if present");
    addOption<bool>(false, "active_stereo", "use active_stereo or not");
    addOption<bool>(false, "headless", "start without UI, even without a window, run application with --application=\"path/to/app\"");
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

bool VROptions::hasOption(string name) {
    return options.find(name) != options.end();
}

void VROptions::printHelp() {
    cout << "\nConfiguration options:\n";
    for (const auto& [name, description] : descriptions) {
        cout << "  --" << setw(15) << left << name << " : " << description
             << " (default: " << options[name] << ")\n";
    }
    cout << endl;
}

void VROptions::parse(int _argc, char** _argv) {
    argc = _argc;
    argv = _argv;

    cout << " parse command line options ( " << argc << " parameter(s) )" << endl;
    for (int i=0; i<argc; i++) cout << "  " << i+1 << ") '" << _argv[i] << "'" << endl;

    try {
        for (int i = 1; i < argc; ++i) {
            string arg = argv[i];

            if (arg == "--help") {
                printHelp();
                exit(1);
            }

            if (arg.rfind("--", 0) == 0) {
                string name = arg.substr(2);
                string value;

                // Check if next argument exists and is not another option
                if (i + 1 < argc && argv[i + 1][0] != '-') {
                    value = argv[++i];
                } else {
                    value = "true"; // boolean flag
                }

                options[name] = value;
            } else {
                // positional argument assumed to be application
                options["application"] = arg;
            }
        }
    } catch(exception& e) {
        cout << "VROptions::parse exception: " << e.what() << endl;
        exit(1);
    } catch(...) {
        cout << "VROptions::parse unknown exception" << endl;
        printHelp();
        exit(1);
    }

    // for testing
    //setOption<bool>("headless", true);
    //setOption<bool>("maximized", true);

    cout << endl;
}
