#include "VRNetwork.h"
#include "core/utils/VRManager.cpp"
#include "core/networking/VRPing.h"
#include "core/gui/VRGuiUtils.h"

#include <libssh/libssh.h>
#include <boost/filesystem.hpp>

/*

IMPORTANT: ..not compiling? you need to install the libssh-dev package!
open a terminal
sudo apt-get install libssh-dev

*/

using namespace OSG;

//template<> class VRManager<VRNetworkNode>;
//template<> VRNetworkNodePtr VRManager<VRNetworkNode>::add(string name);

VRNetwork::VRNetwork() : VRManager("Network") {}
VRNetwork::~VRNetwork() { cout << "~VRNetwork"; }


VRNetworkNode::VRNetworkNode(string name) : VRManager("NetworkNode") {
    setNameSpace("NetworkNode");
    setName(name);

    store("address", &address);
    store("user", &user);
    regStorageUpdateFkt( VRFunction<int>::create("network_node_update", boost::bind(&VRNetworkNode::update, this)) );
}

VRNetworkNode::~VRNetworkNode() {}

VRNetworkNodePtr VRNetworkNode::create(string name) { return VRNetworkNodePtr( new VRNetworkNode(name) ); }

string VRNetworkNode::getAddress() { return address; }
string VRNetworkNode::getUser() { return user; }

void VRNetworkNode::setAddress(string s) { address = s; update(); }
void VRNetworkNode::setUser(string s) { user = s; update(); }

string VRNetworkNode::getStatNode() { return stat_node; }
string VRNetworkNode::getStatSSH() { return stat_ssh; }
string VRNetworkNode::getStatSSHkey() { return stat_ssh_key; }

void VRNetworkNode::set(string a, string u) {
    address = a;
    user = u;
    update();
}

class VRSSHSession {
    private:
        ssh_session session;
        string stat;
        string stat_key;
        string address;
        string user;

        string keyFolder = "/.ssh/";
        string privKeyPath = "id_rsa";
        string pubKeyPath = "id_rsa.pub";
        //string privKeyPath = "~/.ssh/id_rsa";
        //string pubKeyPath = "~/.ssh/id_rsa.pub";

    public:
        VRSSHSession(string a, string u) {
            address = a;
            user = u;

            session = ssh_new();
            if (session == NULL) { stat = "ssh init failed"; return; }

            int verbosity = SSH_LOG_NOLOG; // SSH_LOG_PROTOCOL
            int port = 22;
            ssh_options_set(session, SSH_OPTIONS_HOST, address.c_str());
            ssh_options_set(session, SSH_OPTIONS_LOG_VERBOSITY, &verbosity);
            ssh_options_set(session, SSH_OPTIONS_PORT, &port);

            int rc = ssh_connect(session);
            if (rc != SSH_OK) { stat = ssh_get_error(session); ssh_free(session); return; }

            stat = verify_knownhost();
            if (stat != "ok") return;
            stat = auth_user();
        }

        ~VRSSHSession() {
            ssh_disconnect(session);
            ssh_free(session);
        }

        static shared_ptr<VRSSHSession> open(string a, string u) { return shared_ptr<VRSSHSession>( new VRSSHSession(a,u) ); }

        string verify_knownhost() {
            int state = ssh_is_server_known(session);
            unsigned char *hash = NULL;
            int hlen = ssh_get_pubkey_hash(session, &hash);
            if (hlen < 0) return "Got no hash from host.";
            char* hexa = ssh_get_hexa(hash, hlen);
            string Hash(hexa);
            free(hexa);
            free(hash);

            string err;
            switch (state) {
                case SSH_SERVER_KNOWN_OK: return "ok";
                case SSH_SERVER_ERROR: return ssh_get_error(session);
                case SSH_SERVER_KNOWN_CHANGED:
                    err = "Host key for server changed."; break;
                case SSH_SERVER_FOUND_OTHER:
                    err = "The host key for this server was not found but an other type of key exists."; break;
                case SSH_SERVER_FILE_NOT_FOUND:
                    err = "Could not find known host file."; break;
                case SSH_SERVER_NOT_KNOWN:
                    err = "The server is unknown."; break;
            }

            if (err.size()) {
                askUser(err, "If you trust the host key (hash):\n" + Hash + "\nthe host will be added to your list of known hosts.");
                if (ssh_write_knownhost(session) < 0) return strerror(errno);
            }

            return "ok";
        }

        string auth_user() {
            int rc = ssh_userauth_publickey_auto(session, NULL, NULL);
            if (rc != SSH_AUTH_SUCCESS) return ssh_get_error(session);
            return "ok";
        }

        bool hasLocalKey() {
            string kf = getenv("HOME") + keyFolder;
            if (!boost::filesystem::exists(kf))
                boost::filesystem::create_directory(kf);
            return boost::filesystem::exists(kf+privKeyPath);
        }

        string checkLocalKeyPair() {
            if (hasLocalKey()) return "ok";
            cout << "Warning! generate new ssh key pair!" << endl;
            string kf = getenv("HOME") + keyFolder;

            ssh_key priv, pub;

            int rc = ssh_pki_generate(SSH_KEYTYPE_RSA, 2048, &priv);
            if (rc != SSH_OK) return strerror(errno);
            rc = ssh_pki_export_privkey_to_pubkey(priv, &pub);
            if (rc != SSH_OK) return strerror(errno);
            rc = ssh_pki_export_privkey_file(priv, NULL, NULL, NULL, (kf+privKeyPath).c_str());
            if (rc != SSH_OK) return strerror(errno);
            rc = ssh_pki_export_pubkey_file(pub, (kf+pubKeyPath).c_str());
            if (rc != SSH_OK) return strerror(errno);

            ssh_key_free(priv);
            ssh_key_free(pub);
            return "ok";
        }

        string exec_cmd(string cmd) {
            ssh_channel channel = ssh_channel_new(session);
            if (channel == NULL) return strerror(errno);

            int rc = ssh_channel_open_session(channel);
            if (rc != SSH_OK) {
                ssh_channel_free(channel);
                return strerror(errno);
            }

            rc = ssh_channel_request_exec(channel, cmd.c_str());
            if (rc != SSH_OK) {
                ssh_channel_close(channel);
                ssh_channel_free(channel);
                return strerror(errno);
            }

            ssh_channel_send_eof(channel);
            ssh_channel_close(channel);
            ssh_channel_free(channel);

            return "ok";
        }

        void distrib_key() {
            if (stat == "ok") { stat_key = "ok"; return; }
            string pw = askUserPass("No pubkey acces, enter password for " + user + "@" + address + " to distribute the key");
            int rc = ssh_userauth_password(session, NULL, pw.c_str());
            if (rc != SSH_AUTH_SUCCESS) { stat_key = strerror(errno); return; }

            stat_key = checkLocalKeyPair();
            if (stat_key != "ok") return;

            string pkey = getenv("HOME") + keyFolder + pubKeyPath;
            ifstream file(pkey.c_str());
            string key_data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            file.close();

            stat_key = exec_cmd("echo \"" + key_data + "\" >> ~/.ssh/authorized_keys");
        }

        operator bool() const { return (stat == "ok"); }
        string getStat() { return stat; }
        string getKeyStat() { return stat_key; }
};


void VRNetworkNode::distributeKey() {
    if (stat_node != "ok") return;
    auto ssh = VRSSHSession::open(address, user);
    ssh->distrib_key();
    stat_ssh = ssh->getStat();
    stat_ssh_key = ssh->getKeyStat();
    ssh.reset();
    update();
}

void VRNetworkNode::update() {
    cout << "update network node " << address << endl;
    stat_node = "ok";
    stat_ssh = "";
    stat_ssh_key = "";

    VRPing p;
    if ( !p.start(address, "22", 1) ) { stat_node = "ping failed"; return; }

    auto ssh = VRSSHSession::open(address, user);
    stat_ssh = ssh->getStat();
    stat_ssh_key = ssh->getKeyStat();
}

VRNetworkSlave::VRNetworkSlave(string name) {
    setNameSpace("NetworkNode");
    setName(name);

    regStorageUpdateFkt( VRFunction<int>::create("network_slave_update", boost::bind(&VRNetworkSlave::update, this)) );
}

VRNetworkSlave::~VRNetworkSlave() {}

VRNetworkSlavePtr VRNetworkSlave::create(string name) { return VRNetworkSlavePtr( new VRNetworkSlave(name) ); }

void VRNetworkSlave::update() {}
