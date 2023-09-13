#include "coreDumpHandler.h"

#ifndef _WIN32
#include <sys/resource.h>
#endif
#include <iostream>
#include <boost/filesystem.hpp>
#include <signal.h>

using namespace std;

const char* dumpPath = "/tmp";
const char* dumpFile = "/tmp/core";


void testSignal(int sig) {
#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
    kill(getpid(), sig);
#endif
}

#ifndef _WIN32
extern "C" void coreDump(int sig) {
    cout << "\n called PolyVR coreDump" << endl;
    static bool once = true;
    if (!once) {
        cout << "\n final signal " << endl;
        signal(sig, NULL);
    } else {
        cout << "\n dump core, check in /var/lib/apport/coredump" << endl;
        //cout << "\n dump core to " << dumpFile << endl;
        //boost::filesystem::current_path(dumpPath); // TODO: doesnt work anymore, coredumps are in /var/lib/apport/coredump
    }
    once = false;
#ifndef __EMSCRIPTEN__
    kill(getpid(), sig);
#endif
}
#endif

void clearDumpFiles() {
#ifndef WASM
    remove(dumpFile); // in /tmp

    string file = boost::filesystem::current_path().string()+"/core";
    remove(file.c_str()); // in current folder
#endif
}

void enableCoreDump(bool b) {
    if (!b) return;
#ifndef _WIN32
    // remove possible core file
    clearDumpFiles();

    // Enable core dumps
    struct rlimit corelim;
    corelim.rlim_cur = RLIM_INFINITY;
    corelim.rlim_max = RLIM_INFINITY;
    if (setrlimit (RLIMIT_CORE, &corelim) != 0) cerr << "Couldn't set core limit\n";

    signal(SIGSEGV, &coreDump);
    signal(SIGFPE, &coreDump);
    signal(SIGABRT, &coreDump);
    signal(SIGTRAP, &coreDump);
    signal(SIGBUS, &coreDump);

    /*signal(SIGHUP, &coreDump);
    signal(SIGINT, &coreDump);
    signal(SIGQUIT, &coreDump);
    signal(SIGILL, &coreDump);
    signal(SIGTRAP, &coreDump);
    signal(SIGABRT, &coreDump);
    signal(SIGFPE, &coreDump);
    signal(SIGKILL, &coreDump);
    signal(SIGUSR1, &coreDump);
    signal(SIGSEGV, &coreDump);
    signal(SIGUSR2, &coreDump);
    signal(SIGPIPE, &coreDump);
    signal(SIGALRM, &coreDump);
    signal(SIGTERM, &coreDump);
    signal(SIGSTKFLT, &coreDump);
    signal(SIGCHLD, &coreDump);
    signal(SIGCONT, &coreDump);
    signal(SIGSTOP, &coreDump);
    signal(SIGTSTP, &coreDump);
    signal(SIGTTIN, &coreDump);
    signal(SIGTTOU, &coreDump);
    signal(SIGURG, &coreDump);
    signal(SIGXCPU, &coreDump);
    signal(SIGXFSZ, &coreDump);
    signal(SIGVTALRM, &coreDump);
    signal(SIGPROF, &coreDump);
    signal(SIGWINCH, &coreDump);
    signal(SIGIO, &coreDump);
    signal(SIGPWR, &coreDump);*/
#endif
}
