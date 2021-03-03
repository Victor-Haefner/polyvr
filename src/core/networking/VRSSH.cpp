#include "VRSSH.h"
#include "core/gui/VRGuiUtils.h"
#include "core/utils/toString.h"
#include "core/utils/system/VRSystem.h"

#ifndef _WIN32
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#ifndef INADDR_NONE
#define INADDR_NONE (in_addr_t)-1
#endif

VRSSHSession::VRSSHSession(string a, string u) {
    address = a;
    user = u;

    session = libssh2_session_init();
    if (session == NULL) { stat = "ssh init failed"; return; }

    stat = connect_session();
    if (stat != "ok") return;

    stat = verify_knownhost();
    if (stat != "ok") return;

    stat = auth_user();
    if (stat == "ok") getRemoteOS();
}

VRSSHSession::~VRSSHSession() {
    libssh2_session_disconnect(session, "Client disconnecting normally");
    libssh2_session_free(session);
    //libssh2_exit();
}

shared_ptr<VRSSHSession> VRSSHSession::open(string a, string u) { return shared_ptr<VRSSHSession>( new VRSSHSession(a,u) ); }

string VRSSHSession::lastError(int pos) {
    char* msg;
    libssh2_session_last_error(session, &msg, NULL, 0);
    return "Error: " + toString(pos) + " " + string(msg);
}

string VRSSHSession::connect_session() { // close(sock);
    auto sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == -1) return "Error opening socket";

    struct sockaddr_in server;
    server.sin_family = AF_INET;

    struct hostent* he;
    if ( (he = gethostbyname(address.c_str()) ) == NULL ) return "Could not resolve hostname " + address;
    memcpy(&server.sin_addr, he->h_addr_list[0], he->h_length);

    server.sin_port = htons(22);
    if (connect(sock, (struct sockaddr*)(&server), sizeof(struct sockaddr_in)) != 0)
        return "Failed to connect";

    int rc = libssh2_session_handshake(session, sock);
    if (rc) return lastError(1);
    return "ok";
}

string VRSSHSession::verify_knownhost() { // TODO
    return "ok";

    string Hash = libssh2_hostkey_hash(session, LIBSSH2_HOSTKEY_HASH_SHA1);
    size_t HKlen; int HKtype;
    auto HKey = libssh2_session_hostkey(session, &HKlen, &HKtype);
    // LIBSSH2_HOSTKEY_TYPE_UNKNOWN 0
    // LIBSSH2_HOSTKEY_TYPE_RSA 1
    // LIBSSH2_HOSTKEY_TYPE_DSS 2

    // #define LIBSSH2_KNOWNHOST_KEY_RSA1     (1<<18)
    // #define LIBSSH2_KNOWNHOST_KEY_SSHRSA   (2<<18)
    // #define LIBSSH2_KNOWNHOST_KEY_SSHDSS   (3<<18)

    string err;
    LIBSSH2_KNOWNHOSTS* nh = libssh2_knownhost_init(session);
    if (!nh) return lastError(2);
    string kf = getenv("HOME") + keyFolder + "known_hosts";
    int rc = libssh2_knownhost_readfile(nh, kf.c_str(), LIBSSH2_KNOWNHOST_FILE_OPENSSH);
    if (rc < 0) return lastError(3);

    int mode = LIBSSH2_KNOWNHOST_TYPE_PLAIN | LIBSSH2_KNOWNHOST_KEYENC_RAW;
    struct libssh2_knownhost *host;
    int check = libssh2_knownhost_checkp(nh, address.c_str(), 22, Hash.c_str(), Hash.size(), mode, &host);
    if (check == LIBSSH2_KNOWNHOST_CHECK_NOTFOUND) err = "Host not found";
    if (check == LIBSSH2_KNOWNHOST_CHECK_MISMATCH) ; //err = "Host mismatch"; // TODO
    if (check == LIBSSH2_KNOWNHOST_CHECK_FAILURE) err = "Host check failure";

    if (err.size()) {
        bool ok = askUser(err, "If you trust the host key (hash):\n" + Hash + "\nthe host will be added to your list of known hosts.");
        if (!ok) return "Host not trusted by user";
        int mode2 = LIBSSH2_KNOWNHOST_TYPE_PLAIN | LIBSSH2_KNOWNHOST_KEYENC_RAW | ((HKtype+1) << LIBSSH2_KNOWNHOST_KEY_SHIFT); // LIBSSH2_KNOWNHOST_KEY_RSA1, LIBSSH2_KNOWNHOST_KEY_SSHRSA or LIBSSH2_KNOWNHOST_KEY_SSHDSS
        rc = libssh2_knownhost_addc(nh, address.c_str(), 0, HKey, HKlen, NULL, 0, mode2, NULL );
        if (rc < 0) return lastError(31);
        rc = libssh2_knownhost_writefile(nh, kf.c_str(), LIBSSH2_KNOWNHOST_FILE_OPENSSH);
        if (rc < 0) return lastError(32);
    }

    libssh2_knownhost_free(nh);
    return "ok";
}

string VRSSHSession::auth_user() {
    string kf = getenv("HOME") + keyFolder;
    string pk1 = kf + pubKeyPath;
    string pk2 = kf + privKeyPath;
    int rc = libssh2_userauth_publickey_fromfile(session, user.c_str(), pk1.c_str(), pk2.c_str(), NULL);
    if (rc < 0) return lastError(4);
    return "ok";
}

bool VRSSHSession::hasLocalKey() {
    string kf = getenv("HOME") + keyFolder;
    if (!exists(kf)) makedir(kf);
    return exists(kf+privKeyPath);
}

void VRSSHSession::createLocalKey() {
    if (hasLocalKey()) return;
    string kf = getenv("HOME") + keyFolder;
    systemCall("ssh-keygen -b 2048 -t rsa -f ~/.ssh/id_rsa -q -N \"\"");
}

string VRSSHSession::checkLocalKeyPair() { // TODO, deprecated?
    if (hasLocalKey()) return "ok";
    cout << "Warning! generate new ssh key pair!" << endl;
    string kf = getenv("HOME") + keyFolder;

    /*ssh_key priv, pub;

    int rc = ssh_pki_generate(SSH_KEYTYPE_RSA, 2048, &priv);
    if (rc != SSH_OK) return strerror(errno);
    rc = ssh_pki_export_privkey_to_pubkey(priv, &pub);
    if (rc != SSH_OK) return strerror(errno);
    rc = ssh_pki_export_privkey_file(priv, NULL, NULL, NULL, (kf+privKeyPath).c_str());
    if (rc != SSH_OK) return strerror(errno);
    rc = ssh_pki_export_pubkey_file(pub, (kf+pubKeyPath).c_str());
    if (rc != SSH_OK) return strerror(errno);

    ssh_key_free(priv);
    ssh_key_free(pub);*/
    return "ok";
}

string VRSSHSession::exec_cmd(string cmd, bool read) {
    auto channel = libssh2_channel_open_session(session);
    if (channel == NULL) return lastError(6);

    cout << "exec ssh cmd:\n" << cmd << endl;
    int rc = libssh2_channel_exec(channel, cmd.c_str());
    if (rc) return lastError(61);

    if (read) {
        string res;
        char buffer[256];
        do {
            rc = libssh2_channel_read(channel, buffer, sizeof(buffer) );
            res += string(buffer, rc);
        } while (rc > 0);
        if (rc < 0) return lastError(63);
        return res;
    }

    rc = libssh2_channel_send_eof(channel);
    if (rc < 0) return lastError(64);
    rc = libssh2_channel_close(channel);
    if (rc < 0) return lastError(65);
    int exisStatus = libssh2_channel_get_exit_status(channel);
    libssh2_channel_free(channel);
    if (exisStatus == 0) return "ok";
    else return string("failed with: ") + toString(exisStatus);
}

void VRSSHSession::distrib_key() {
    if (stat == "ok") { stat_key = "ok"; return; }
    string pw = askUserPass("No pubkey acces, enter password for " + user + "@" + address + " to distribute the key");
    int rc = libssh2_userauth_password(session, user.c_str(), pw.c_str());
    if (rc < 0) { stat_key = lastError(5); return; }

    if (os == "") getRemoteOS();

    stat_key = checkLocalKeyPair();
    if (stat_key != "ok") return;

    string pkey = getenv("HOME") + keyFolder + pubKeyPath;
    ifstream file(pkey.c_str());
    string key_data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    key_data.pop_back(); // remove newline char
    file.close();

    if (os == "nix") stat_key = exec_cmd("echo \"" + key_data + "\" >> ~/.ssh/authorized_keys");
    if (os == "win") stat_key = exec_cmd("echo|set /p=\"" + key_data + "\" > .ssh/authorized_keys");
}

string VRSSHSession::getRemoteOS() {
    //cout << "SSH test remote OS: " << user + "@" + address << endl;
    auto test1 = exec_cmd("printf 'pvr-os-detection'");
    if (test1 == "pvr-os-detection") os = "nix";
    else os = "win";
    //cout << " result, remote OS is: " << os << endl;
    return os;
}

VRSSHSession::operator bool() const { return (stat == "ok"); }
string VRSSHSession::getStat() { return stat; }
string VRSSHSession::getKeyStat() { return stat_key; }






