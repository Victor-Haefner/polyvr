#include "VRProfibusClient.h"
#include "snap7.h"

#include <thread>
#include <iostream>

using namespace OSG;

struct VRProfinetClient::Data {
     TS7Client* Client = 0;
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

bool VRProfinetClient::isConnected() { return data && data->Client ? data->Client->Connected() : false; }

void VRProfinetClient::connect(string address, int rack, int slot) {
    data->Address = address;
    data->Client->ConnectTo(data->Address.c_str(), rack, slot);
    if (!isConnected()) return;

    printf("  PDU Requested  : %d bytes\n", data->Client->PDURequested());
    printf("  PDU Negotiated : %d bytes\n", data->Client->PDULength());
    //data->pollThread = thread( [&](){ while (data->doPoll) mg_mgr_poll(&data->mgr, 1000); } );
}

string VRProfinetClient::read(int db, int offset, int length, string dbType) {
    if (!isConnected()) return "";
    if (dbType == "merker"){
        cout << "VRProfinetClient::read merkerbase " << db << " at " << offset << " length " << length << ", connected? " << isConnected() << endl;
        int r = data->Client->MBRead(offset, length, data->Buffer);
        cout << " return " << r << ", data: ";
        for (int i=0; i<length; i++) cout << " " << (int)data->Buffer[i];
        cout << endl;
        return string((char*)data->Buffer, length);
    }
    if (dbType == "input"){
        cout << "VRProfinetClient::read inputbase " << db << " at " << offset << " length " << length << ", connected? " << isConnected() << endl;
        int r = data->Client->EBRead(offset, length, data->Buffer);
        cout << " return " << r << ", data: ";
        for (int i=0; i<length; i++) cout << " " << (int)data->Buffer[i];
        cout << endl;
        return string((char*)data->Buffer, length);
    }
    if (dbType == "output"){
        cout << "VRProfinetClient::read outputbase " << db << " at " << offset << " length " << length << ", connected? " << isConnected() << endl;
        int r = data->Client->ABRead(offset, length, data->Buffer);
        cout << " return " << r << ", data: ";
        for (int i=0; i<length; i++) cout << " " << (int)data->Buffer[i];
        cout << endl;
        return string((char*)data->Buffer, length);
    }
    cout << "VRProfinetClient::read db " << db << " at " << offset << " length " << length << ", connected? " << isConnected() << endl;
    int r = data->Client->DBRead(db, offset, length, data->Buffer);
    cout << " return " << r << ", data: ";
    for (int i=0; i<length; i++) cout << " " << (int)data->Buffer[i];
    cout << endl;
    return string((char*)data->Buffer, length);
}

void VRProfinetClient::write(int db, int offset, string val, string dbType) {
    if (!isConnected()) return;
    if (dbType == "merker") {
        data->Client->MBWrite(offset, val.size(), (void*)val.c_str());
        return;
    }
    if (dbType == "input") {
        data->Client->EBWrite(offset, val.size(), (void*)val.c_str());
        return;
    }
    if (dbType == "output") {
        data->Client->ABWrite(offset, val.size(), (void*)val.c_str());
        return;
    }
    data->Client->DBWrite(db, offset, val.size(), (void*)val.c_str());
}

float VRProfinetClient::toFloat(string bytes) {
    if (bytes.size() != 4) return 0;
    float f;
    char* fm = (char*)&f;
    fm[0] = bytes[3];
    fm[1] = bytes[2];
    fm[2] = bytes[1];
    fm[3] = bytes[0];
    return f;
}

int VRProfinetClient::toInt(string bytes) {
    if (bytes.size() != 2) return 0;
    short f;
    char* fm = (char*)&f;
    fm[0] = bytes[1];
    fm[1] = bytes[0];
    return f;
}


bool VRProfinetClient::readBool(int db, int pos, int bit, string dbType) {
    byte B = read(db, pos, 1, dbType)[0];
    cout << " readBool " << (int)B << endl;
    return ((B >> bit) & 0x01);
}

int VRProfinetClient::readInt(int db, int pos, string dbType) {
    return toInt( read(db, pos, 2, dbType) );
}

float VRProfinetClient::readFloat(int db, int pos, string dbType) {
    return toFloat( read(db, pos, 4, dbType) );
}


void VRProfinetClient::writeBool(int db, int pos, int bit, bool val, string dbType) {
    byte B = read(db, pos, 1, dbType)[0];
    if (val) B |= (1 << bit);
    else B &= ~(1 << bit);
    string f = string((char*)&B, 1);
    write(db, pos, f, dbType);
}

void VRProfinetClient::writeInt(int db, int pos, int val, string dbType) {
    string f = string((char*)&val, 2);
    write(db, pos, f, dbType);
}

void VRProfinetClient::writeFloat(int db, int pos, float val, string dbType) {
    string f = string((char*)&val, 4);
    write(db, pos, f, dbType);
}


