#include "VRSystem.h"
#include <stdlib.h>
#include <iostream>
#include <boost/filesystem.hpp>
#ifndef WITHOUT_EXECINFO
#include <execinfo.h>
#endif
#include <stdio.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <chrono>

void printBacktrace() {
#ifndef WITHOUT_EXECINFO
    void *buffer[100];
    char **strings;

    int nptrs = backtrace(buffer, 100);
    printf("backtrace() returned %d addresses\n", nptrs);

    strings = backtrace_symbols(buffer, nptrs);
    if (strings != NULL) {
        for (int j = 0; j < nptrs; j++) printf("%s\n", strings[j]);
        free(strings);
    }
#endif
}

bool exists(string path) { return boost::filesystem::exists(path); }

bool makedir(string path) {
    bool res = false;
    if (path == "") return true;
    try { res = boost::filesystem::create_directory(path); }
    catch(...) { cout << "ERROR: makedir failed when trying to create directory '" + path + "'" << endl; }
    return res;
}

bool removeFile(string path) { return boost::filesystem::remove(path); }
string canonical(string path) { return boost::filesystem::canonical(path).string(); }

bool isFile(string path) { return boost::filesystem::is_regular_file(path); }
bool isFolder(string path) { return boost::filesystem::is_directory(path); }

string getFileName(string path, bool withExtension) {
    string fname;
    size_t sp = path.rfind('/');
    if (sp == string::npos) fname = path;
    else fname = path.substr(sp+1, path.size());
    if (!withExtension) {
        sp = fname.rfind('.');
        if (sp != string::npos) fname = fname.substr(0, sp);
    }
    return fname;
}

string getFileExtension(string path) {
    size_t sp = path.rfind('.');
    if (sp == string::npos) return "";
    return path.substr(sp);
}

string getFolderName(string path) {
    size_t sp = path.rfind('/');
    if (sp == string::npos) return ".";
    return path.substr(0, sp);
}

vector<string> openFolder(string folder) {
    vector<string> res;
    if ( !exists( folder ) ) return res;
    if ( !isFolder( folder ) ) return res;

#ifndef WASM
    boost::filesystem::directory_iterator End; // default construction yields past-the-end
    for ( boost::filesystem::directory_iterator itr( folder ); itr != End; ++itr ) {
        string name = itr->path().filename().string();
        res.push_back( name );
    }
#endif
    return res;
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

typedef chrono::time_point<chrono::high_resolution_clock, chrono::nanoseconds> timePoint;
timePoint globalStartTime;

void initTime() {
    timePoint tp = chrono::high_resolution_clock::now();
    globalStartTime = tp;
}

long long getTime() {
    auto elapsed = chrono::high_resolution_clock::now() - globalStartTime;
    return chrono::duration_cast<chrono::microseconds>(elapsed).count();
}

void fileReplaceStrings(string filePath, string oldString, string newString) {
    auto escapeSpecialChar = [&](string& str, char c1, const string& c2) {
        vector<size_t> positions; // get all singe quote positions to escape
        for (size_t i=0; i<str.size(); i++) {
            char c = str[i];
            if (c == c1) positions.push_back(i);
        }

        if (positions.size() == 0) return;

        for (size_t i = positions.size()-1; i>=0; i--) {
            str.replace(positions[i], 1, c2);
            if (i == 0) break;
        }
    };

    auto escapeChars = [&](string& str) {
        escapeSpecialChar(str, '\t', "\\x09");
        escapeSpecialChar(str, '\n', "\\n");
        escapeSpecialChar(str, '\'', "\\x27");

        vector<size_t> positions; // get all char positions to escape
        for (size_t i=0; i<str.size(); i++) {
            char c = str[i];
            if (c == '/') positions.push_back(i);
            if (c == '\'') positions.push_back(i);
            if (c == '(') positions.push_back(i);
            if (c == ')') positions.push_back(i);
        }

        if (positions.size() == 0) return;

        for (size_t i = positions.size()-1; i>=0; i--) {
            str.insert(positions[i], "\\");
            if (i == 0) break;
        }
        cout << " escapeChars " << str << endl;
    };

    escapeChars(oldString);
    escapeChars(newString);

    string cmd = "sed -i 's/" + oldString + "/" + newString + "/g' "+filePath;
    systemCall(cmd);
    cout << "fileReplaceStrings " << cmd << endl;
}








