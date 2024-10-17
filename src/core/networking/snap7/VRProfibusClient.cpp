#include "VRProfibusClient.h"
#include "snap7.h"
#include "core/utils/toString.h"
#include "core/utils/VRGlobals.h"
#include "core/utils/system/VRSystem.h"

#include "core/utils/Thread.h"
#include <iostream>

using namespace OSG;

struct VRProfinetClient::Data {
     TS7Client* Client = 0;
     string Address;     // PLC IP Address
	 byte Buffer[65536]; // 64 K buffer

	 Thread writeThread;
	 bool active = true;
	 VRMutex mtx;
};

VRProfinetClient::VRProfinetClient() {
    data = new Data();
    data->Client = new TS7Client();
    //data->Client->SetAsCallback(CliCompletion,NULL);

    data->writeThread = Thread( "profinetClient", [&](){ while (data->active) processWriteQueue(); } );
}

VRProfinetClient::~VRProfinetClient() {
    disconnect();
    if (data) {
        data->active = false;
        data->writeThread.join();
        if (data->Client) delete data->Client;
        delete data;
    }
}

VRProfinetClientPtr VRProfinetClient::create() { return VRProfinetClientPtr( new VRProfinetClient() ); }
VRProfinetClientPtr VRProfinetClient::ptr() { return shared_from_this(); }

void VRProfinetClient::disconnect() {
    VRLock lock(data->mtx);
    if (data && data->Client) data->Client->Disconnect();
}

bool VRProfinetClient::isConnected() {
    VRLock lock(data->mtx);
    return data && data->Client ? data->Client->Connected() : false;
}

void VRProfinetClient::connect(string address, int rack, int slot) {
    VRLock lock(data->mtx);
    data->Address = address;
    data->Client->ConnectTo(data->Address.c_str(), rack, slot);
    if (!isConnected()) return;

    printf("  PDU Requested  : %d bytes\n", data->Client->PDURequested());
    printf("  PDU Negotiated : %d bytes\n", data->Client->PDULength());
}

string VRProfinetClient::read(int db, int offset, int length, string dbType) {
    if (!isConnected()) return "";

    VRLock lock(data->mtx);
    string key = dbType + toString(db);
    if (blocks.count(key)) {
        auto& bl = blocks[key];
        if (bl.timestamp == VRGlobals::CURRENT_FRAME) {
            if (bl.offset <= offset && bl.size+bl.offset >= offset+length) {
                string s = subString(bl.data, offset-bl.offset, length);
                return s;
            }
        }
    }

    int r = 0;
    if (dbType == "merker") r = data->Client->MBRead(offset, length, data->Buffer);
    else if (dbType == "input") r = data->Client->EBRead(offset, length, data->Buffer);
    else if (dbType == "output") r = data->Client->ABRead(offset, length, data->Buffer);
    else r = data->Client->DBRead(db, offset, length, data->Buffer);

    return string((char*)data->Buffer, length);
}

void VRProfinetClient::write(int db, int offset, string val, string dbType) {
    if (!isConnected()) return;

    string key = dbType + toString(db);
    VRLock lock(data->mtx);
    if (blocks.count(key)) {
        auto& bl = blocks[key];
        if (bl.timestamp == VRGlobals::CURRENT_FRAME) {
            if (bl.offset <= offset && bl.size+bl.offset >= offset+val.length()) {
                for (int i=0; i<val.length(); i++) bl.data[offset-bl.offset+i] = val[i];
                bl.needsPush = true;
                return;
            }
        }
    }

    writeBus(db, offset, val, dbType);
}

void VRProfinetClient::writeBus(int db, int offset, string val, string dbType) {
    if (!isConnected()) return;
    VRLock lock(data->mtx);
    if (dbType == "merker") data->Client->MBWrite(offset, val.size(), (void*)val.c_str());
    else if (dbType == "input") data->Client->EBWrite(offset, val.size(), (void*)val.c_str());
    else if (dbType == "output") data->Client->ABWrite(offset, val.size(), (void*)val.c_str());
    else data->Client->DBWrite(db, offset, val.size(), (void*)val.c_str());
}

void VRProfinetClient::processWriteQueue() {
    {
        VRLock lock(data->mtx);
        for (auto& b : blocks) {
            auto& bl = b.second;
            if (!bl.needsPush) continue;
            writeBus(bl.db, bl.offset, bl.data, bl.dbType);
            bl.needsPush = false;
        }
    }
    doFrameSleep(0,60.0);
}

void VRProfinetClient::readBlock(int db, int pos, int size, string dbType) {
    VRLock lock(data->mtx);
    string key = dbType + toString(db);
    blocks[key] = Block();
    auto& bl = blocks[key];
    bl.data = read(db, pos, size, dbType);
    bl.timestamp = VRGlobals::CURRENT_FRAME;
    bl.offset = pos;
    bl.size = size;
    bl.db = db;
    bl.dbType = dbType;
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

short VRProfinetClient::toShort(string bytes) {
    if (bytes.size() != 2) return 0;
    short f;
    char* fm = (char*)&f;
    fm[0] = bytes[1];
    fm[1] = bytes[0];
    return f;
}

int VRProfinetClient::toInt(string bytes) {
    if (bytes.size() != 4) return 0;
    int f;
    char* fm = (char*)&f;
    fm[0] = bytes[3];
    fm[1] = bytes[2];
    fm[2] = bytes[1];
    fm[3] = bytes[0];
    return f;
}


bool VRProfinetClient::readBool(int db, int pos, int bit, string dbType) {
    byte B = read(db, pos, 1, dbType)[0];
    //cout << " readBool " << (int)B << endl;
    bool b = ((B >> bit) & 0x01);
    return b;
}

short VRProfinetClient::readShort(int db, int pos, string dbType) {
    return toShort( read(db, pos, 2, dbType) );
}

int VRProfinetClient::readInt(int db, int pos, string dbType) {
    return toInt( read(db, pos, 4, dbType) );
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

void VRProfinetClient::writeShort(int db, int pos, short val, string dbType) {
    string f = string((char*)&val, 2);
    swap(f[0], f[1]);
    write(db, pos, f, dbType);
}

void VRProfinetClient::writeInt(int db, int pos, int val, string dbType) {
    string f = string((char*)&val, 4);
    swap(f[0], f[3]);
    swap(f[1], f[2]);
    write(db, pos, f, dbType);
}

void VRProfinetClient::writeFloat(int db, int pos, float val, string dbType) {
    string f = string((char*)&val, 4);
    swap(f[0], f[3]);
    swap(f[1], f[2]);
    write(db, pos, f, dbType);
}


