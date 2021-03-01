#include "VRMapManager.h"

#include "core/utils/toString.h"
#include "core/utils/VRFunction.h"
#include "core/utils/system/VRSystem.h"
#include "core/networking/rest/VRRestResponse.h"
#include "core/networking/rest/VRRestClient.h"

#include <fstream>
#include <OpenSG/OSGVector.h>

using namespace OSG;

VRMapManager::VRMapManager() {}
VRMapManager::~VRMapManager() {}

VRMapManagerPtr VRMapManager::create() { return VRMapManagerPtr( new VRMapManager() ); }
VRMapManagerPtr VRMapManager::ptr() { return static_pointer_cast<VRMapManager>(shared_from_this()); }

void VRMapManager::setServer(string s) { server = s; }
void VRMapManager::setVault(string v) { vault = v; }

string VRMapManager::getMap(double N, double E, double S, VRMessageCbPtr mcb) {
    string filename = constructFilename(N,E,S);
    if (exists(filename)) {
        if (mcb) (*mcb)(filename);
        return filename;
    }
    else requestFile(filename, N,E,S, mcb);
    return filename;
}

// --= utilities =--

void VRMapManager::requestFile(string filename, double N, double E, double S, VRMessageCbPtr mcb) {
    string sN = toString(N,3);
    string sE = toString(E,3);
    string sS = toString(S,3);
    string req = server+"Topology_GetMap.php?N="+sN+"&E="+sE+"&S="+sS;  // N=%.3f&E=%.3f&S=%.3f" % (N,E,S)
    cout << " VRMapManager request: " << req << endl;

    // launch get request
    if (!client) client = VRRestClient::create();

    auto cb = [&](VRRestResponsePtr response, string filename, VRMessageCbPtr mcb) {
        //cout << " response: " << response->getStatus() << endl;
        cout << " map data response, data size: " << response->getData().size() << endl;
        cout << " store map data in: " << filename << endl;

        // store result in file 'filename'
        storeFile(filename, response->getData());
        (*mcb)(filename);
    };

    if (mcb) client->getAsync(req, VRRestCb::create("asyncGet", bind(cb, placeholders::_1, filename, mcb)));
    else     client->get(req);
}

void VRMapManager::storeFile(const string& filename, const string& data) {
    ofstream f;
    f.open(filename);
    f << data;
    f.close();
}

string VRMapManager::constructFilename(double N, double E, double S) {
    //cout << "VRMapManager::constructFilename " << Vec3d(N, E, S) << endl;
    string sN = toString(N,3);
    string sE = toString(E,3);
    string sS = toString(S,3);

    string folder = vault+"/"+sS;
    string filename = folder+"/N"+sN+"E"+sE+"S"+sS+".hgt";
    if (!exists(folder)) makedir(folder);
    return filename;
}


