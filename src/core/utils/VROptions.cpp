#include "VROptions.h"

#include <boost/program_options/parsers.hpp>
#include <iostream>
#include <fstream>
#include <iomanip>

using namespace std;
namespace bpo = boost::program_options;

VROptions::VROptions() : desc("Configuration ") {
    desc.add_options() ("help", "show possible options");

    addOption<int>(0, "shadowType");
    addOption<int>(1024, "shadowMapSize");
    addOption<float>(0.5, "shadowColor");

    addOption<string>("", "setupCfg");

    addOption<int>(0, "setup", "wich hardware setup to choose (0,1,2), will be changed to setup config files soon");
    addOption<int>(-1, "scene", "wich scenes to choose (0,1,2), will be changed to scene config files soon");
    addOption<float>(1.0, "scale", "scale the real environement in the virtual world");
    //("stereo", bpo::value<bool>(), "use stereo or not")
    addOption<bool>(false, "active_stereo", "use active_stereo or not");
    addOption<bool>(false, "swap_buffer", "use active_stereo or not");
    addOption<float>(0, "cave_offsetX", "X offset for the cave model");
    addOption<float>(0, "cave_offsetY", "Y offset for the cave model");
    addOption<float>(0, "cave_offsetZ", "Z offset for the cave model");
    //("projection", bpo::value<bool>(), "use projections or not")
    addOption<string>("", "graphene_path", "path to load from");
    addOption<bool>(false, "graphene_wired", "shaded or wired");
    addOption<float>(1, "graphene_scale", "scale the amplitude of the wave function");

    addOption<int>(1, "ann_inputs", "number of inputs for the ann");
    addOption<int>(1, "ann_trigger", "input number of the ann used for trigger maximum");
    addOption<bool>(true, "dragndrop", "activates drag and drop");
    addOption<float>(1, "menu_size", "size of menu");

    addOption<bool>(false, "deferredShading", "activates deferredShading");

    addOption<int>(0, "http_soc_port", "port of http socket");
    addOption<string>("", "http_soc_addr", "server addr of http socket");

    addOption<bool>(false, "vrpn", "enable vrpn");
}

void VROptions::operator= (VROptions v) {;}

VROptions* VROptions::get() {
    static VROptions* singleton = 0;
    if (singleton == 0) singleton = new VROptions();
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
