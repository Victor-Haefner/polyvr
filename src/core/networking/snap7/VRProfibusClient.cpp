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

float VRProfinetClient::toFloat(string bytes) {
    float f;
    f = *(float*)&bytes[0];
    return f;
    //bytes.reverse();
    //return struct.unpack('f', bytes)[0];
}

int VRProfinetClient::toInt(string bytes) {
    short f;
    f = *(short*)&bytes[0];
    return f;
    //bytes.reverse();
    //return struct.unpack('h', bytes)[0];
}


bool VRProfinetClient::readBool(int db, int pos, int bit) {
    byte B = read(db, pos, 1)[0];
    return bool( (B & bit) != 0 );
}

int VRProfinetClient::readInt(int db, int pos) {
    return toInt( read(db, pos, 2) );
}

float VRProfinetClient::readFloat(int db, int pos) {
    return toFloat( read(db, pos, 4) );
}


void VRProfinetClient::writeBool(int db, int pos, int bit, bool val) {
    byte B = read(db, pos, 1)[0];
    if (val) B |= (1 << bit);
    else B &= ~(1 << bit);
    string f = string((char*)&B, 1);
    write(db, pos, f);
}

void VRProfinetClient::writeInt(int db, int pos, int val) {
    string f = string((char*)&val, 2);
    write(db, pos, f);
}

void VRProfinetClient::writeFloat(int db, int pos, float val) {
    string f = string((char*)&val, 4);
    write(db, pos, f);
}






