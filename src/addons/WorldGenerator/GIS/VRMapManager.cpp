#include "VRMapManager.h"

#include "core/utils/toString.h"
#include "core/utils/VRFunction.h"
#include "core/utils/system/VRSystem.h"
#include "core/scene/VRScene.h"
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

void VRMapManager::addMapType(int ID, string vault, string servScript, string fileExt, string format) {
    mapTypes[ID].ID = ID;
    mapTypes[ID].vault = vault;
    mapTypes[ID].servScript = servScript;
    mapTypes[ID].fileExt = fileExt;
    mapTypes[ID].format = format;
}

VRMapDescriptorPtr VRMapManager::getMap(double N, double E, double S, vector<int> types, VRMapCbPtr mcb, bool doCache) {
    VRMapDescriptorPtr data = VRMapDescriptor::create(N,E,S);
    //cout << "VRMapManager::getMap " << Vec3d(N,E,S) << endl;

    for (auto mapType : types) {
        string filename = constructFilename(N,E,S, mapType);
        data->setMap(mapType, filename);
    }

    for (auto mapType : types) {
        string filename = data->getMapPath(mapType);
        if (!exists(filename)) {
            requestFile(filename, N,E,S, mapType, types, mcb);
            if (!mcb) data->setMap(mapType, filename);
            else {
                data->setCompleteness(false);
                break;
            }
        }
    }

    if (mcb && data->isComplete()) {
        auto fkt = VRUpdateCb::create("map manager job", bind(&VRMapManager::triggerCB, this, mcb, data));
        VRScene::getCurrent()->queueJob(fkt);
    }
    return data;
}

// --= utilities =--

void VRMapManager::handleRequestAnswer(VRRestResponsePtr response, string filename, VRMapCbPtr mcb, double N, double E, double S, vector<int> types) {
    //cout << " response: " << response->getStatus() << endl;
    //cout << " map data response, data size: " << response->getData().size() << endl;
    //cout << " store map data in: " << filename << endl;

    // store result in file 'filename'
    storeFile(filename, response->getData());

    if (!exists(filename)) { cout << "VRMapManager Warning! store file '" << filename << "' failed!" << endl; return; }
    if (mcb) getMap(N, E, S, types, mcb);
}

void VRMapManager::requestFile(string filename, double N, double E, double S, int mapType, vector<int> types, VRMapCbPtr mcb) {
    string sN = toString(N,3);
    string sE = toString(E,3);
    string sS = toString(S,3);

    string script = mapTypes[mapType].servScript;
    string format = mapTypes[mapType].format;
    string req = server+script+"?N="+sN+"&E="+sE+"&S="+sS+"&F="+format;  // N=%.3f&E=%.3f&S=%.3f" % (N,E,S)
    cout << " VRMapManager request: " << req << endl;

    // launch get request
    if (!client) client = VRRestClient::create();

    if (mcb) client->getAsync(req, VRRestCb::create("asyncGet", bind(&VRMapManager::handleRequestAnswer, this, placeholders::_1, filename, mcb, N, E, S, types)));
    else handleRequestAnswer(client->get(req), filename, mcb, N, E, S, types);
}

void VRMapManager::triggerCB(VRMapCbPtr mcb, VRMapDescriptorPtr data) {
    (*mcb)(data);
}

void VRMapManager::storeFile(const string& filename, const string& data) {
    if (data == "") return;
    ofstream f;
    f.open(filename);
    f << data;
    f.close();
}

string VRMapManager::constructFilename(double N, double E, double S, int mapType) {
    //cout << "VRMapManager::constructFilename " << Vec3d(N, E, S) << endl;
    string sN = toString(N,3);
    string sE = toString(E,3);
    string sS = toString(S,3);

    string vault = mapTypes[mapType].vault;
    string fileExt = mapTypes[mapType].fileExt;
    string folder = vault+"/"+sS;
    string filename = folder+"/N"+sN+"E"+sE+"S"+sS+fileExt;
    if (!exists(folder)) makedir(folder);
    return filename;
}



// ----------------------- VRMapDescriptor -----------------------

VRMapDescriptor::VRMapDescriptor() {}
VRMapDescriptor::~VRMapDescriptor() {}

VRMapDescriptorPtr VRMapDescriptor::create() { return VRMapDescriptorPtr( new VRMapDescriptor() ); }

VRMapDescriptorPtr VRMapDescriptor::create(double n, double e, double s) {
    auto d = create();
    d->setParameters(n,e,s);
    return d;
}

VRTexturePtr VRMapDescriptor::getMap(int i) {
    if (layers.count(i)) return layers[i].tex;
    return 0;
}

string VRMapDescriptor::getMapPath(int i) {
    if (layers.count(i)) return layers[i].path;
    return "";
}

Vec3d VRMapDescriptor::getParameters() { return Vec3d(N, E, S); }

void VRMapDescriptor::setMap(int i, string s) { layers[i].path = s; }
void VRMapDescriptor::setParameters(double n, double e, double s) { N = n; E = e; S = s; }

void VRMapDescriptor::setCompleteness(bool c) { complete = c; }
bool VRMapDescriptor::isComplete() { return complete; }
