#include "coreDumpHandler.h"

#ifndef _WIN32
#include <sys/resource.h>
#endif
#include <iostream>
#include <boost/filesystem.hpp>
#include <signal.h>

using namespace std;

const char* dumpPath = "/tmp";
const char* dumpFile = "/tmp/pvr_core";

#ifndef _WIN32
extern "C" void coreDump(int sig) {
    cout << "\n dump core to " << dumpFile << endl;
    boost::filesystem::current_path(dumpPath);
    kill(getpid(), SIGABRT);
}
#endif

void enableCoreDump(bool b) {
    if (!b) return;
#ifndef _WIN32
    // remove possible core file
    remove(dumpFile);

    // Enable core dumps
    struct rlimit corelim;
    corelim.rlim_cur = -1;
    corelim.rlim_max = -1;
    if (setrlimit (RLIMIT_CORE, &corelim) != 0) cerr << "Couldn't set core limit\n";

    signal(SIGSEGV, &coreDump);
    signal(SIGFPE, &coreDump);

    /*signal(SIGHUP, &coreDump);
    signal(SIGINT, &coreDump);
    signal(SIGQUIT, &coreDump);
    signal(SIGILL, &coreDump);
    signal(SIGTRAP, &coreDump);
    signal(SIGABRT, &coreDump);
    signal(SIGBUS, &coreDump);
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
