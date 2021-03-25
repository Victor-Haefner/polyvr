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
void VRMapManager::setVault(string v) { vault = v; }

string VRMapManager::getMap(double N, double E, double S, VRMapCbPtr mcb) {
    string filename = constructFilename(N,E,S);
    if (exists(filename)) {
        if (mcb) {
            auto data = VRMapDescriptor::create(N,E,S,filename);
            (*mcb)(data);
        }
        return filename;
    }
    else requestFile(filename, N,E,S, mcb);
    return filename;
}

// --= utilities =--

void VRMapManager::triggerCB(VRMapCbPtr mcb, VRMapDescriptorPtr data) {
    (*mcb)(data);
}

void VRMapManager::requestFile(string filename, double N, double E, double S, VRMapCbPtr mcb) {
    string sN = toString(N,3);
    string sE = toString(E,3);
    string sS = toString(S,3);
    string req = server+"Topology_GetMap.php?N="+sN+"&E="+sE+"&S="+sS;  // N=%.3f&E=%.3f&S=%.3f" % (N,E,S)
    cout << " VRMapManager request: " << req << endl;

    // launch get request
    if (!client) client = VRRestClient::create();

    auto cb = [](VRRestResponsePtr response, VRMapManager* mm, string filename, VRMapCbPtr mcb, double N, double E, double S) {
        //cout << " response: " << response->getStatus() << endl;
        cout << " map data response, data size: " << response->getData().size() << endl;
        cout << " store map data in: " << filename << endl;

        // store result in file 'filename'
        mm->storeFile(filename, response->getData());
        auto data = VRMapDescriptor::create();
        data->setParameters(N, E, S);
        data->setMap(0, filename);
        auto fkt = VRUpdateCb::create("map manager job", bind(&VRMapManager::triggerCB, mm, mcb, data));
        VRScene::getCurrent()->queueJob(fkt);
    };

    if (mcb) client->getAsync(req, VRRestCb::create("asyncGet", bind(cb, placeholders::_1, this, filename, mcb, N, E, S)));
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



// ----------------------- VRMapDescriptor -----------------------

VRMapDescriptor::VRMapDescriptor() {}
VRMapDescriptor::~VRMapDescriptor() {}

VRMapDescriptorPtr VRMapDescriptor::create() { return VRMapDescriptorPtr( new VRMapDescriptor() ); }

VRMapDescriptorPtr VRMapDescriptor::create(double n, double e, double s, string f) {
    auto d = create();
    d->setMap(0, f);
    d->setParameters(n,e,s);
    return d;
}

string VRMapDescriptor::getMap(int i) { return layers[i]; }
Vec3d VRMapDescriptor::getParameters() { return Vec3d(N, E, S); }

void VRMapDescriptor::setMap(int i, string s) { layers[i] = s; }
void VRMapDescriptor::setParameters(double n, double e, double s) { N = n; E = e; S = s; }

