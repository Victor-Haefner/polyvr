#include "VRSSH.h"
#include "core/gui/VRGuiUtils.h"

#include <boost/filesystem.hpp>

VRSSHSession::VRSSHSession(string a, string u) {
    address = a;
    user = u;

    session = libssh2_session_init();
    /*if (session == NULL) { stat = "ssh init failed"; return; }

    int verbosity = SSH_LOG_NOLOG; // SSH_LOG_PROTOCOL
    int port = 22;
    ssh_options_set(session, SSH_OPTIONS_HOST, address.c_str());
    ssh_options_set(session, SSH_OPTIONS_LOG_VERBOSITY, &verbosity);
    ssh_options_set(session, SSH_OPTIONS_PORT, &port);

    int rc = ssh_connect(session);
    if (rc != SSH_OK) { stat = ssh_get_error(session); ssh_free(session); return; }

    stat = verify_knownhost();
    if (stat != "ok") return;
    stat = auth_user();*/
}

VRSSHSession::~VRSSHSession() {
    libssh2_session_disconnect(session, "Client disconnecting normally");
    libssh2_session_free(session);
}

shared_ptr<VRSSHSession> VRSSHSession::open(string a, string u) { return shared_ptr<VRSSHSession>( new VRSSHSession(a,u) ); }

string VRSSHSession::verify_knownhost() {
    /*int state = ssh_is_server_known(session);
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

    return "ok";*/
}

string VRSSHSession::auth_user() {
    /*int rc = ssh_userauth_publickey_auto(session, NULL, NULL);
    if (rc != SSH_AUTH_SUCCESS) return ssh_get_error(session);
    return "ok";*/
}

bool VRSSHSession::hasLocalKey() {
    string kf = getenv("HOME") + keyFolder;
    if (!boost::filesystem::exists(kf))
        boost::filesystem::create_directory(kf);
    return boost::filesystem::exists(kf+privKeyPath);
}

string VRSSHSession::checkLocalKeyPair() {
    /*if (hasLocalKey()) return "ok";
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
    return "ok";*/
}

string VRSSHSession::exec_cmd(string cmd, bool read) {
    /*ssh_channel channel = ssh_channel_new(session);
    if (channel == NULL) return strerror(errno);

    auto retErr = [&](int step = 2) {
        if (step > 1) ssh_channel_close(channel);
        if (step > 0) ssh_channel_free(channel);
        return strerror(errno);
    };

    int rc = ssh_channel_open_session(channel);
    if (rc != SSH_OK) return retErr(1);

    cout << "exec ssh cmd:\n" << cmd << endl;
    rc = ssh_channel_request_exec(channel, cmd.c_str());
    if (rc != SSH_OK) return retErr();

    if (read) {
        char buffer[256]; int nbytes;
        do {
            nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 0);
            if (fwrite(buffer, 1, nbytes, stdout) != nbytes) return retErr();
        } while (nbytes > 0);
        if (nbytes < 0) return retErr();
    }

    sleep(3);

    ssh_channel_send_eof(channel);
    ssh_channel_close(channel);
    ssh_channel_free(channel);
    return "ok";*/
}

void VRSSHSession::distrib_key() {
    /*if (stat == "ok") { stat_key = "ok"; return; }
    string pw = askUserPass("No pubkey acces, enter password for " + user + "@" + address + " to distribute the key");
    int rc = ssh_userauth_password(session, NULL, pw.c_str());
    if (rc != SSH_AUTH_SUCCESS) { stat_key = strerror(errno); return; }

    stat_key = checkLocalKeyPair();
    if (stat_key != "ok") return;

    string pkey = getenv("HOME") + keyFolder + pubKeyPath;
    ifstream file(pkey.c_str());
    string key_data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    stat_key = exec_cmd("echo \"" + key_data + "\" >> ~/.ssh/authorized_keys");*/
}

VRSSHSession::operator bool() const { return (stat == "ok"); }
string VRSSHSession::getStat() { return stat; }
string VRSSHSession::getKeyStat() { return stat_key; }






