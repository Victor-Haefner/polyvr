#include "VRSystem.h"
#include <stdlib.h>
#include <iostream>
#include <boost/filesystem.hpp>

bool exists(string path) { return boost::filesystem::exists(path); }
bool makedir(string path) { return boost::filesystem::create_directory(path); }
bool removeFile(string path) { return boost::filesystem::remove(path); }
string canonical(string path) { return boost::filesystem::canonical(path).string(); }

string getFileName(string path) {
    size_t sp = path.rfind('/');
    if (sp == string::npos) return path;
    return path.substr(sp+1, path.size());
}

string getFolderName(string path) {
    size_t sp = path.rfind('/');
    if (sp == string::npos) return "";
    return path.substr(0, sp);
}


int systemCall(string cmd) {
    return system(cmd.c_str());
}

bool compileCodeblocksProject(string path) {
    if (!exists(path)) { cout << "compiling codeblocks project '" << path << "' failed, path not found!"; return false; }
    string cmd = "codeblocks --no-ipc --no-splash-screen --target='Release' --build ";
    cmd += path;
    cout << "compile codeblocks project: " << cmd << endl;
    return systemCall(cmd) == 0;
}
