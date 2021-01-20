#include "VRMapManager.h"

#include "core/utils/toString.h"
#include "core/utils/system/VRSystem.h"
#include "core/networking/rest/VRRestResponse.h"
#include "core/networking/rest/VRRestClient.h"

#include <fstream>
#include <OpenSG/OSGVector.h>

using namespace OSG;

template<> string typeName(const VRMapManager& p) { return "MapManager"; }

VRMapManager::VRMapManager() {}
VRMapManager::~VRMapManager() {}

VRMapManagerPtr VRMapManager::create() { return VRMapManagerPtr( new VRMapManager() ); }
VRMapManagerPtr VRMapManager::ptr() { return static_pointer_cast<VRMapManager>(shared_from_this()); }

void VRMapManager::setServer(string s) { server = s; }
void VRMapManager::setVault(string v) { vault = v; }

string VRMapManager::getMap(double N, double E, double S) {
    //cout << "VRMapManager::getMap " << Vec3d(N, E, S) << endl;
    string sN = toString(N,3);
    string sE = toString(E,3);
    string sS = toString(S,3);

    string folder = vault+"/"+sS;
    string filename = folder+"/N"+sN+"E"+sE+"S"+sS+".hgt";  // "/%.3f/N%.3fE%.3fS%.3f.hgt' % (S, N, E, S)
    //cout << " filename: " << filename << endl;
    if (exists(filename)) return filename;

    string req = server+"Topology_GetMap.php?N="+sN+"&E="+sE+"&S="+sS;  // N=%.3f&E=%.3f&S=%.3f" % (N,E,S)
    cout << " VRMapManager request: " << req << endl;

    // launch get request
    if (!client) client = VRRestClient::create();
    auto response = client->get(req);
    //cout << " response: " << response->getStatus() << endl;
    cout << " map data response, data size: " << response->getData().size() << endl;
    cout << " store map data in: " << filename << endl;

    // store result in file 'filename'
    if (!exists(folder)) makedir(folder);
    ofstream f;
    f.open(filename);
    f << response->getData();
    f.close();
    return filename;
}
