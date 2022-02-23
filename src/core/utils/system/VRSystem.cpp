#include "VRSystem.h"
#include "../VRTimer.h"
#include "../toString.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <iostream>
#include <boost/filesystem.hpp>
#ifndef WITHOUT_EXECINFO
#include <execinfo.h>
#endif
#include <stdio.h>

#ifndef _WIN32
#include <unistd.h>
#endif

#ifdef _WIN32
#define _AMD64_
#include <fileapi.h>
#endif

#include <thread>
#include <chrono>

#ifdef WASM
#include <sys/stat.h>
#endif

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

bool exists(string path) {
#ifdef WASM
	struct stat buffer;
	return (stat (path.c_str(), &buffer) == 0);
#else
	return boost::filesystem::exists(path);
#endif
}

#ifdef WASM
bool isFile(string path) { return false; }
bool isFolder(string path) { return false; }
#else
bool isFile(string path) { return boost::filesystem::is_regular_file(path); }
bool isFolder(string path) { return boost::filesystem::is_directory(path); }
#endif

bool makedir(string path) {
    cout << "makedir: " << path << endl;
    bool res = false;
    if (path == "") return true;
#ifdef WASM
    auto folders = splitString(path, '/');
    string tmp = "";
    for (auto f : folders) {
    	//cout << " folder: " << f << ", exists? " << exists(tmp+f) << endl;
        if (!exists(tmp+f)) {
	    //cout << "mkdir " << tmp+f << endl;
	    int result = mkdir((tmp+f).c_str(), 0777);
            if (result < 0) cout << " -> errno: " << errno << ", result: " << result << ", errno str: " << strerror(errno) << endl;
	}
        tmp += f+"/";
    }
#else
    try { res = boost::filesystem::create_directories(path); }
    catch(const boost::filesystem::filesystem_error& e) { cout << "ERROR: makedir failed when trying to create directory '" + path + "', " << e.code().message() << endl; }
    catch(...) { cout << "ERROR: makedir failed when trying to create directory '" + path + "'" << endl; }
#endif
    return res;
}

bool removeFile(string path) {
#ifdef WASM
    return bool(std::remove(path.c_str()) == 0);
#else
    return boost::filesystem::remove(path);
#endif
}

string canonical(string path) {
#ifdef WASM
    return path;
#else
    return boost::filesystem::canonical(path).string();
#endif
}

string absolute(string path) {
#ifdef WASM
    return path;
#else
    return boost::filesystem::absolute(path).string();
#endif
}

bool isSamePath(string path1, string path2) {
#ifdef WASM
    return false;
#else
    return boost::filesystem::equivalent(path1, path2);
#endif
}

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

string createTempFile() {
    /*char tmpname[L_tmpnam];
    char* r = std::tmpnam(tmpname);
    if (r == 0) cout << "create temp file failed" << endl;
    return string() + tmpname;*/
#ifdef _WIN32
    static char P_tmpdir[MAX_PATH + 1] = { 0 };
    if (!P_tmpdir[0]) GetTempPath(sizeof(P_tmpdir), P_tmpdir);
#endif
    return string() + P_tmpdir + "/exec_out_file" + toString(time(0)) + "_" + toString(rand());
}

string readFileContent(string fileName) {
    ifstream file(fileName, ios::in | ios::binary);
    string result;
    if (file) {
        while (!file.eof()) result.push_back(file.get());
        file.close();
    }
    return result;
}

string ssystem(const char* command) {
    string tmpFile = createTempFile();
    string scommand = command;
    string cmd = scommand + " >> " + tmpFile;
    int r = std::system(cmd.c_str());
    if (r != 0) cout << "system call did not return 0 (" << r << ")" << endl;
    string result = readFileContent(tmpFile);
    remove(tmpFile.c_str());
    return result;
}

string systemCall(string cmd) {
    //cmd += " > callO"
    return ssystem(cmd.c_str());
    //return system(cmd.c_str());
}

bool compileCodeblocksProject(string path) {
    if (!exists(path)) { cout << "compiling codeblocks project '" << path << "' failed, path not found!"; return false; }
    string cmd = "codeblocks --no-ipc --no-splash-screen --target='Release' --build ";
    cmd += path;
    cout << "compile codeblocks project: " << cmd << endl;
    systemCall(cmd);
    return true;
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

template <typename T>
using duration = std::chrono::duration<T, std::milli>;

void doFrameSleep(double tFrame, double fps) {
    double fT = 1000.0 / fps;             // target frame duration in ms
    double sT = max(fT - tFrame, 0.0);    // time to sleep
    if (sT <= 0) return;

    // efficient sleep
    double precisionBuffer = 1.5;
    if (sT-precisionBuffer > 0) {
        VRTimer timer;
        timer.start();
        duration<double> T(sT-precisionBuffer);
        std::this_thread::sleep_for(T);
        sT = max(fT - tFrame - timer.stop(), 0.0); // remaining time to sleep
        if (sT <= 0) return;
    }

    // precision sleep
    static constexpr duration<double> MinSleepDuration(0);
    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    while (duration<double>(std::chrono::high_resolution_clock::now() - start).count() < sT) {
        std::this_thread::sleep_for(MinSleepDuration);
    }
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

#ifdef WASM
namespace boost {
	namespace filesystem {
		BOOST_FILESYSTEM_DECL int path::compare(path const& p) const BOOST_NOEXCEPT
		{
		    return bool(string() == p.string());
		}
	}
}
#endif






