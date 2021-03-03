#ifndef VRSSH_H_INCLUDED
#define VRSSH_H_INCLUDED

#include <memory>
#include <string>
#include <libssh2.h>

using namespace std;

/*

IMPORTANT: ..not compiling? you need to install the libssh2-1-dev package!
open a terminal
sudo apt-get install libssh2-1-dev

*/

class VRSSHSession {
    private:
        LIBSSH2_SESSION* session;
        string stat;
        string stat_key;
        string address;
        string user;

        string os = "";

        string keyFolder = "/.ssh/";
        string privKeyPath = "id_rsa";
        string pubKeyPath = "id_rsa.pub";
        //string privKeyPath = "~/.ssh/id_rsa";
        //string pubKeyPath = "~/.ssh/id_rsa.pub";

        string lastError(int pos);

    public:
        VRSSHSession(string a, string u);

        ~VRSSHSession();

        static shared_ptr<VRSSHSession> open(string a, string u);

        string getRemoteOS();
        string verify_knownhost();
        string auth_user();
        string connect_session();

        bool hasLocalKey();
        void createLocalKey();
        string checkLocalKeyPair();

        void distrib_key();
        string exec_cmd(string cmd, bool read = true);

        operator bool() const;
        string getStat();
        string getKeyStat();
};


#endif // VRSSH_H_INCLUDED
