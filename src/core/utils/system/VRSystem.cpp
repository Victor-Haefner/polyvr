#include "VRSystem.h"
#include <stdlib.h>
#include <iostream>

int systemCall(string cmd) {
    return system(cmd.c_str());
}

bool compileCodeblocksProject(string path) {
    string cmd = "codeblocks --no-ipc --no-splash-screen --target='Release' --build ";
    cmd += path;
    cout << "compile codeblocks project: " << cmd << endl;
    return systemCall(cmd) == 0;
}

