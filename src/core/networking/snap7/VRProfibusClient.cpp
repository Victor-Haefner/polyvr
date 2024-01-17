#include "VRProfibusClient.h"
#include "snap7.h"

#include <thread>

using namespace OSG;

struct VRProfinetClient::Data {
     TS7Client* Client = 0;
     bool connected = false;
     string Address;     // PLC IP Address
	 byte Buffer[65536]; // 64 K buffer
};

VRProfinetClient::VRProfinetClient() {
    data = new Data();
    data->Client = new TS7Client();
    //data->Client->SetAsCallback(CliCompletion,NULL);
}

VRProfinetClient::~VRProfinetClient() {
    disconnect();
    if (data && data->Client) delete data->Client;
    if (data) delete data;
}

VRProfinetClientPtr VRProfinetClient::create() { return VRProfinetClientPtr( new VRProfinetClient() ); }
VRProfinetClientPtr VRProfinetClient::ptr() { return shared_from_this(); }

void VRProfinetClient::disconnect() {
    if (data && data->Client) data->Client->Disconnect();
}

bool VRProfinetClient::isConnected() { return data ? data->connected : false; }

void VRProfinetClient::connect(string address, int rack, int slot) {
    data->Address = address;
    data->connected = data->Client->ConnectTo(data->Address.c_str(), rack, slot);
    if (!data->connected) return;

    printf("  PDU Requested  : %d bytes\n", data->Client->PDURequested());
    printf("  PDU Negotiated : %d bytes\n", data->Client->PDULength());
    //data->pollThread = thread( [&](){ while (data->doPoll) mg_mgr_poll(&data->mgr, 1000); } );
}

string VRProfinetClient::read(int db, int offset, int length) {
    if (!data->connected) return "";
    data->Client->DBRead(db, offset, length, data->Buffer);
    return string((char*)data->Buffer, length);
}

void VRProfinetClient::write(int db, int offset, string val) {
    if (!data->connected) return;
    data->Client->DBWrite(db, offset, val.size(), (void*)val.c_str());
}






