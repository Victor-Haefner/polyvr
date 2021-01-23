#include "VRHDLC.h"
#include "core/utils/VRFunction.h"
#include "core/utils/toString.h"
#include "core/utils/system/VRSystem.h"

#include <boost/filesystem.hpp>

#include <iostream>
#include <sstream>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h> // memset
#include <unistd.h> // close
#include <sys/ioctl.h> //controll RTS & DTR
#include <asm/ioctls.h>
#include <asm/termbits.h>

#include "core/gui/VRGuiManager.h"
#include "core/gui/VRGuiConsole.h"

#define WARN(x) \
VRGuiManager::get()->getConsole( "Errors" )->write( x+"\n" );

using namespace OSG;

template<> string typeName(const VRHDLC& t) { return "HDLC"; }


int VRSerial::set_interface_attribs (int fd, int speed, int parity) {
    /*struct termios tty;
    memset (&tty, 0, sizeof tty);
    if (tcgetattr (fd, &tty) != 0) { cout << "VRSerial: set_interface_attribs failed!\n"; return -1; }*/

    struct termios2 tty;
    ioctl(fd, TCGETS2, &tty);
    // disable IGNBRK for mismatched speed tests; otherwise receive break as \000 chars
    tty.c_lflag = 0;                // no signaling chars, no echo, no canonical processing

    tty.c_iflag &= ~IGNBRK;         // disable break processing
    tty.c_iflag &=~(INLCR|ICRNL);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

    tty.c_oflag = 0;                // no remapping, no delays
    tty.c_oflag &=~(ONLCR|OCRNL);

    tty.c_cc[VMIN]  = 0;            // read doesn't block
    tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

    tty.c_cflag &= ~CBAUD;
    tty.c_cflag |= BOTHER;
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
    tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls, enable reading
    tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
    tty.c_cflag |= parity;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    tty.c_ispeed = speed;
    tty.c_ospeed = speed;
    ioctl(fd, TCSETS2, &tty);
    ioctl(fd, TIOCEXCL);

    /*cfsetospeed (&tty, speed);
    cfsetispeed (&tty, speed);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
    // disable IGNBRK for mismatched speed tests; otherwise receive break as \000 chars
    tty.c_iflag &= ~IGNBRK;         // disable break processing
    tty.c_lflag = 0;                // no signaling chars, no echo, no canonical processing
    tty.c_oflag = 0;                // no remapping, no delays
    tty.c_cc[VMIN]  = 0;            // read doesn't block
    tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout
    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl
    tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls, enable reading
    tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
    tty.c_cflag |= parity;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    if (tcsetattr (fd, TCSANOW, &tty) != 0) { cout << "VRSerial: set_interface_attribs failed!\n"; return -1; }*/
    return 0;
}

VRSerial::VRSerial(string interfaceName) {
    interface = interfaceName;
    fd = open(interfaceName.c_str(), O_RDWR | O_NOCTTY | O_SYNC | O_NONBLOCK);
    if (fd < 0) {
        cout << "Connecting to VRSerial port " << interfaceName << " failed!\n";
        cout << " Reason: " << strerror(errno) << endl;
        WARN( string("Serial connection to " + interfaceName + " failed with '") + strerror(errno) + "'");
        return;
    }
    set_interface_attribs(fd, 256000, 0);  // set speed to 115,200 bps, 8n1 (no parity)
}

VRSerial::~VRSerial() {}

VRSerialPtr VRSerial::create(string interfaceName) { return VRSerialPtr(new VRSerial(interfaceName)); }

vector<unsigned char> VRSerial::read() {
    int n = ::read(fd, buf, sizeof buf);
    if (n < 0) return vector<unsigned char>();
    string s(buf, n);
    return asHex(s);
}

void VRSerial::write(vector<unsigned char> data) {
    string s = asString(data);
    size_t r = ::write (fd, s.c_str(), s.size());
    if (r < 0) return; // only to get rid of the warning..
}

bool VRSerial::good() {
    return exists(interface);
}


VRHDLC::VRHDLC() {}
VRHDLC::~VRHDLC() {}
VRHDLCPtr VRHDLC::create() { return VRHDLCPtr(new VRHDLC()); }

bool VRHDLC::isIdle() { return idle; }

void VRHDLC::pauseSend(int T) {
    pausingTimeS = time(0);
	pausingDurationS = T;
}

void VRHDLC::pauseReceive(int T) {
    pausingTimeR = time(0);
	pausingDurationR = T;
}

string VRHDLC::getInterface() {
    auto interfaces = { "/dev/ttyACM0", "/dev/car-interface" };
    for (auto i : interfaces) {
        if (boost::filesystem::exists(i)) return i;
    }
    for (auto f : openFolder("/dev")) {
        if (startsWith(f, "ttyUSB")) return "/dev/"+f;
    }
    return "";
}

void VRHDLC::connect() {
    cout << "VRHDLC protocol, try connecting.. "; cout.flush();
    string interfaceName = getInterface();
    if (interfaceName == "") {
        cout << "No valid interface found, is the device connected?"; cout.flush();
        idle = true;
        return;
    }
    serial = VRSerial::create(interfaceName);
    if (serial) {
        cout << "Connected to " << interfaceName << endl;
        idle = false;
        lastInput = time(0);
    }
}

bool VRHDLC::connected() {
    if (!serial) return 0;
    if (!serial->good()) return 0;
    return 1;
}

size_t VRHDLC::getLastInput() { return time(0) - lastInput; }

void VRHDLC::handleData() {
    if (verbose) cout << endl << "handleData: " << VRSerial::asHexRepr(serialData) << endl;
    if (callback) (*callback)(serialData);
    serialData.clear();
}

bool VRHDLC::handle(unsigned char c) {
    if (verbose) cout << "B " << serialState << " " << serialData.size() << " " << std::hex << c;
    if (c == 0x7E) { // 126
        if (serialState == "undefined") { serialState = "start"; serialData.clear(); }
        if (serialState == "end") serialState = "start";
        if (serialState == "data") {
            //if (serialData.size() == 0 || serialData == vector<unsigned char>({0xd, 0xa}) || serialData == vector<unsigned char>({0xa})) serialState = "start"; //ignore cariage return or empty data
            if (serialData.size() == 0 || serialData == vector<unsigned char>({0xd, 0xa})) serialState = "start"; //ignore cariage return or empty data
            else {
                serialState = "end";
                handleData();
                return true;
            }
        }
    }
    else if (c == 0x7D) serialState = "escaped"; // 125
    else if (serialState == "start") {
        serialState = "data";
        serialData = {c};
    }
    else if (serialState == "data") serialData.push_back(c);
    else if (serialState == "escaped") {
        serialData.push_back(c ^ 0x20); // XOR 0x20
        serialState = "data";
    }
    if (verbose) cout << " -> " << serialState << " " << VRSerial::asHexRepr(serialData) << " " << serialData.size() << endl;
    return false;
}

bool VRHDLC::process(vector<unsigned char> input) {
    vector<unsigned char> bytes = VRSerial::concat( buffer, input );
    //vector<unsigned char> bytes = input;
    buffer.clear();
    idle = (bytes.size() == 0);
    if (bytes.size() == 0) return true;
    //cout << " " << VRSerial::asHexRepr(input) << endl;
    if (verbose) cout << "A read data! " << serialState << " " << bytes.size() << " " << VRSerial::asHexRepr(bytes) << endl;
    for (unsigned int i = 0; i<bytes.size(); i++) {
        auto b = bytes[i];
        if ( handle(b) ) {
            buffer = VRSerial::slice(bytes, i+1);
            //cout << " store in buffer: " << asHexRepr(buffer) << endl;
            return false;
        }
    }
    return true;
}

bool VRHDLC::readData() {
    if (!serial) return false;
    vector<unsigned char> input = serial->read();
    if (input.size() > 0) lastInput = time(0);

    if (pausingDurationR != 0) {
        int delta = time(0) - pausingTimeR;
        if ( delta <= pausingDurationR) {
            buffer.clear();
            return false;
        }
        pausingDurationR = 0;
    }

    return process(input);
}

void VRHDLC::readAllData() {
    do {
        readData();
    } while( buffer.size() > 0 );
}

void VRHDLC::waitForMessage() { while( readData() ) {}; }

void VRHDLC::setCallback(VRHDLCCbPtr cb) { callback = cb; }

void VRHDLC::sendData(vector<unsigned char> data, bool doWait) {
    if (!serial) return;

    if (pausingDurationS != 0) {
        int delta = time(0) - pausingTimeS;
        if ( delta <= pausingDurationS) return;
        pausingDurationS = 0;
    }

    vector<unsigned char> tmp;
    for (auto d : data) {
        if (d == 0x7D) { tmp.push_back(0x7D); tmp.push_back(0x7D ^ 0x20); } // 0x7D -> 0x7D, 0x7D ^ 0x20
        else if (d == 0x7E) { tmp.push_back(0x7D); tmp.push_back(0x7E ^ 0x20); } // 0x7E -> 0x7D, 0x7E ^ 0x20
        else tmp.push_back(d);
    }
    tmp = VRSerial::concat( VRSerial::concat({0x7E}, tmp) , {0x7E} );
    //if (verbose) cout << " serial send (" << tmp.size() << "): " << asHexRepr( tmp ) << endl;
    //cout << " serial send (" << tmp.size() << "): " << VRSerial::asHexRepr( tmp ) << endl;
    serial->write(tmp);
    if (doWait) waitForMessage();
}

void VRHDLC::runTest() {
    //vector<unsigned char> data = {0x7e, 0x01, 'h', '1', 0x7e, 0xd, 0xa, 0x7e, 0x01, 'h', '2', 0x7e, 0xd, 0xa, 0x7e, 0x01, 'h', '3', 0x7e, 0xd, 0xa};
    /*vector<unsigned char> data = {0x7e, 0x01, 'h', '1', 0x7e, 0xd, 0xa, 0x7e, 0x01, 'h', '2', 0x7e, 0xd, 0xa, 0x7e, 0x01, 'h', '3', 0x7e, 0xd, 0xa};

    auto readTestData = [&]() {
        auto tmp = data;
        data = vector<unsigned char>();
        return tmp;
    };*/
}






string VRSerial::asString(const vector<unsigned char>& data) {
    string res;
    for (auto d : data) res += char(d);
    return res;
}

string VRSerial::asHexRepr(const vector<unsigned char>& data, bool doX, string delim) {
    stringstream ss;
    string x = doX ? "0x" : "";
    for (unsigned int i = 0; i<data.size(); i++) {
        auto d = data[i];
        if (i != 0) ss << delim;
        ss << x << std::hex << (int)(*(unsigned char*)(&d));
    }
    string res = ss.str();
    return res;
}

vector<unsigned char> VRSerial::asHex(const string& data) {
    vector<unsigned char> res;
    for (auto s : data) res.push_back( (unsigned char)(s) );
    return res;
}

vector<unsigned char> VRSerial::slice(vector<unsigned char>& data, int i, int j) {
    if (data.size() == 0) return data;
    if (i < 0 || i >= int(data.size())) i = 0;
    if (j < i) j = data.size();
    //cout << "slice " << asHexRepr(data) << " " << i << " " << j << endl;
    return vector<unsigned char>(&data[i], &data[j]);
}

vector<unsigned char> VRSerial::concat(const vector<unsigned char>& v1, const vector<unsigned char>& v2) {
    if (v1.size() <= 0) return v2;
    if (v2.size() <= 0) return v1;
    vector<unsigned char> res = v1;
    res.insert( res.end(), v2.begin(), v2.end() );
    return res;
}





