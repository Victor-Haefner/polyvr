#include "VROptions.h"

#include <boost/program_options/parsers.hpp>
#include <iostream>
#include <fstream>
#include <iomanip>

using namespace std;
namespace bpo = boost::program_options;

VROptions::VROptions() : desc("Configuration ") {
    desc.add_options() ("help", "show possible options");

    addOption<bool>(true, "dofailcheck");
    addOption<bool>(false, "standalone");
    addOption<string>("", "application");
    addOption<string>("", "decryption");
    addOption<string>("", "setup");
    addOption<bool>(false, "active_stereo", "use active_stereo or not");
}

void VROptions::operator= (VROptions v) {;}

VROptions* VROptions::get() {
    static VROptions* singleton = new VROptions();
    return singleton;
}

void VROptions::parse(int _argc, char** _argv) {
    cout << "Parse command line options\n";
    argc = _argc;
    argv = _argv;

    bpo::store(bpo::parse_command_line(argc, argv, desc), vm);
    bpo::notify(vm);
    if (vm.count("help")) { cout << desc << "\n"; exit(1); }
}
