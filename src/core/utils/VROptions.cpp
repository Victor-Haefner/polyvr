#include "VROptions.h"

#include <boost/program_options/parsers.hpp>
#include <iostream>
#include <fstream>
#include <iomanip>

using namespace std;
namespace bpo = boost::program_options;

VROptions::VROptions() : desc("Configuration ") {
    cout << "Init Options" << endl;
    desc.add_options() ("help", "show possible options");

    addOption<bool>(true, "dofailcheck");
    addOption<bool>(false, "standalone");
    addOption<string>("", "application");
    addOption<string>("", "decryption");
    addOption<string>("", "setup");
    addOption<bool>(false, "active_stereo", "use active_stereo or not");
    cout << " ..done" << endl;
}

void VROptions::operator= (VROptions v) {;}

VROptions* VROptions::get() {
    static VROptions* singleton = new VROptions();
    return singleton;
}

void VROptions::parse(int _argc, char** _argv) {
    cout << "Parse command line options" << endl;
    argc = _argc;
    argv = _argv;
    cout << " argc " << argc << endl;
    for (int i=0; i<argc; i++) cout << "  argv " << i << " '" << _argv[i] << "'" << endl;

    try {
        bpo::store(bpo::parse_command_line(argc, argv, desc), vm);
        bpo::notify(vm);
        if (vm.count("help")) { cout << desc << endl; exit(1); }
    } catch(exception& e) {
        cout << "VROptions::parse exception: " << e.what() << endl;
    } catch(...) {
        cout << "VROptions::parse unknown exception" << endl;
        cout << desc << endl;
    }
    cout << " ..done" << endl;
}
