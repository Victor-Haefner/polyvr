#include "VRSystem.h"
#include "../VRTimer.h"
#include "../toString.h"

#include <stdlib.h>
#include <cstdlib>
#include <sys/stat.h>
#include <errno.h>
#include <iostream>
#include <fstream>
#include <boost/filesystem.hpp>
#ifndef WITHOUT_EXECINFO
#include <execinfo.h>
#include <cxxabi.h>
#endif
#include <stdio.h>
#include <pthread.h>

#ifndef _WIN32
#include <unistd.h>
#endif

#ifdef _WIN32
#define _AMD64_
#include <fileapi.h>
#include <windows.h>
#endif

#ifdef __APPLE__
#include <mach/mach_time.h>
#endif

#include "core/scene/VRThreadManager.h"
#include "core/utils/Thread.h"
#include <chrono>

#ifdef WASM
#include <sys/stat.h>
#endif


void getMemUsage(double& vm_usage, double& resident_set) {
#ifndef WIN32
   using std::ios_base;
   using std::ifstream;
   using std::string;

   vm_usage     = 0.0;
   resident_set = 0.0;

   // 'file' stat seems to give the most reliable results
   //
   ifstream stat_stream("/proc/self/stat",ios_base::in);

   // dummy vars for leading entries in stat that we don't care about
   //
   string pid, comm, state, ppid, pgrp, session, tty_nr;
   string tpgid, flags, minflt, cminflt, majflt, cmajflt;
   string utime, stime, cutime, cstime, priority, nice;
   string O, itrealvalue, starttime;

   // the two fields we want
   //
   unsigned long vsize;
   long rss;

   stat_stream >> pid >> comm >> state >> ppid >> pgrp >> session >> tty_nr
               >> tpgid >> flags >> minflt >> cminflt >> majflt >> cmajflt
               >> utime >> stime >> cutime >> cstime >> priority >> nice
               >> O >> itrealvalue >> starttime >> vsize >> rss; // don't care about the rest

   stat_stream.close();

   long page_size_kb = sysconf(_SC_PAGE_SIZE) / 1024; // in case x86-64 is configured to use 2MB pages
   resident_set = rss * page_size_kb;
   vm_usage     = vsize / 1024.0;
#endif
}

string getSystemVariable(string name) {
    const char* val = getenv(name.c_str());
    return val ? val : "";
}

#ifndef WITHOUT_EXECINFO
std::string demangle(const std::string& mangled_name) {
    int status = 0;

    // Use __cxa_demangle to convert the mangled name to a human-readable format
    char* demangled = abi::__cxa_demangle(mangled_name.c_str(), nullptr, nullptr, &status);

    if (status == 0 && demangled != nullptr) {
        std::string result(demangled);
        free(demangled);  // Free the memory allocated by __cxa_demangle
        return result;
    } else {
        return mangled_name;  // Return the original if demangling fails
    }
}

struct BacktraceEntry {
    bool valid = false;
    string entry;
    vector<string> parts;


    BacktraceEntry(string s) {
        entry = s;

        auto sv = splitString(s, '(');
        if (sv.size() != 2) return;
        string path = sv[0];
        parts.push_back(path);

        sv = splitString(sv[1], ')');
        if (sv.size() != 2) return;
        string call = sv[0];
        //parts.push_back(call);

        auto parts2 = splitString(call, '+');
        if (!parts2.empty()) {
            string mangled_name = parts2[0];
            string offset = (parts2.size() > 1) ? parts2[1] : "";
            string demangled_name = demangle(mangled_name);

            parts.push_back(demangled_name);
            if (!offset.empty()) parts.push_back("Offset: +"+offset);
        }

        parts.push_back(sv[1]);
        valid = true;
    }

    void print() {
        if (!valid) cout << entry << endl;
        else {
            for (auto p : parts) cout << " " << p;
            cout << endl;
        }
    }
};
#endif

void printBacktrace() {
#ifndef WITHOUT_EXECINFO
    void *buffer[100];
    char **strings;

    int nptrs = backtrace(buffer, 100);
    printf("backtrace() returned %d addresses\n", nptrs);

    cout << " thread: " << OSG::VRThreadManager::getThreadName() << endl;

    strings = backtrace_symbols(buffer, nptrs);
    if (strings != NULL) {
        for (int j = 0; j < nptrs; j++) {
            if (strings[j] == 0) continue;
            BacktraceEntry entry(strings[j]);
            entry.print();
        }
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

string readFileContent(string fileName, bool binary) {
    int flags = ios::in;
    if (binary) flags = ios::in | ios::binary;
    ifstream file(fileName, ios::openmode(flags));
    string result;
    if (file) {
        while (!file.eof()) {
            char c = file.get();
            if (c == -1) break; // EOF
            result.push_back(c);
        }
        file.close();
    }
    return result;
}

string ssystem(string command) {
    string tmpFile = createTempFile();
    string cmd = command + " >> " + tmpFile;
    //cout << "systemCall: '" << cmd << "'" << endl;
    int r = std::system(cmd.c_str());
    if (r != 0) cout << "system call did not return 0 (" << r << ")" << endl;
    string result = readFileContent(tmpFile);
    remove(tmpFile.c_str());
    return result;
}

string systemCall(string cmd) {
    return ssystem(cmd);
}

bool compileCodeblocksProject(string path) {
    if (!exists(path)) { cout << "compiling codeblocks project '" << path << "' failed, path not found!"; return false; }
    string cmd = "codeblocks --no-ipc --no-splash-screen --target='Release' --build ";
    cmd += path;
    cout << "compile codeblocks project: " << cmd << endl;
    systemCall(cmd);
    return true;
}

long long cpu_time() {
#ifdef _WIN32
    FILETIME unused;
    FILETIME utime;
    GetThreadTimes(GetCurrentThread(), &unused, &unused, &unused, &utime);
    return ((utime.dwLowDateTime) / 10);
#elif defined(__APPLE__)
    static mach_timebase_info_data_t timebase_info;
    if (timebase_info.denom == 0) mach_timebase_info(&timebase_info);
    uint64_t abs_time = mach_absolute_time();
    return abs_time * timebase_info.numer / timebase_info.denom / 1000;
#else
    thread_local bool initialized(false);
    thread_local clockid_t clock_id;
    thread_local timespec t0;

    if (!initialized) {
        pthread_getcpuclockid(pthread_self(), &clock_id);
        initialized = true;
        clock_gettime(clock_id, &t0);
    }

    timespec result;
    clock_gettime(clock_id, &result);
    return (result.tv_nsec - t0.tv_nsec) / 1000;
#endif
}

typedef chrono::time_point<chrono::high_resolution_clock, chrono::nanoseconds> timePoint;
typedef long long cpuTimePoint;
timePoint globalStartTime;

void initTime() {
    globalStartTime = chrono::high_resolution_clock::now();
}

long long getTime() {
    auto elapsed = chrono::high_resolution_clock::now() - globalStartTime;
    return chrono::duration_cast<chrono::microseconds>(elapsed).count();
}

long long getCPUTime() { // microsecs
    auto elapsed = cpu_time();
    return elapsed;
}

template <typename T>
using duration = std::chrono::duration<T, std::micro>;

void doFrameSleep(double tFrame, double fps) {
    double fT = 1000.0 / fps;             // target frame duration in ms
    double sT = max(fT - tFrame, 0.0);    // time to sleep
    if (sT <= 0) return;

    // efficient sleep
    double precisionBuffer = 0;//1.5;
    if (sT-precisionBuffer > 0) {
        OSG::VRTimer timer;
        timer.start();
        duration<double> T((sT-precisionBuffer)*1000);
        std::this_thread::sleep_for(T);
        sT = max(fT - tFrame - timer.stop(), 0.0); // remaining time to sleep
        if (sT <= 0) return;
    }

    // precision sleep
    /*static constexpr duration<double> MinSleepDuration(0);
    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    while (duration<double>(std::chrono::high_resolution_clock::now() - start).count() < sT) {
        std::this_thread::sleep_for(MinSleepDuration);
    }*/
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

long readAvailableRAM() {
    std::ifstream meminfo("/proc/meminfo");
    std::string line;
    long available_ram = 0;

    while (std::getline(meminfo, line)) {
        if (line.compare(0, 8, "MemFree:") == 0) {
            available_ram = std::stol(line.substr(8)) / 1024.0; // Convert kB to mB
            break;
        }
    }

    return available_ram;
}

void startMemoryDog() {
    return;// TODO: doesnt work..

    static auto dog = new ::Thread("memDog", [](){
        cout << "start memory dog..";
        while (true) {
            long m = readAvailableRAM();
            cout << "mem dog " << m << endl;
            if (m < 500) { cout << "Memory dog, kill system!" << endl; exit(1); }
            Thread::sleepMilli( 1000 );
        }
    });
}
