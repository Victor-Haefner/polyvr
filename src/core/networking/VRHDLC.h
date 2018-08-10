#ifndef VRHDLC_H_INCLUDED
#define VRHDLC_H_INCLUDED

#include "core/networking/VRNetworkingFwd.h"

#include <OpenSG/OSGConfig.h>
#include <string>
#include <vector>
#include <functional>

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRSerial {
    private:
        int fd = 0;

        int set_interface_attribs (int fd, int speed, int parity);

    public:
        VRSerial(string interfaceName);
        ~VRSerial();

        static VRSerialPtr create(string interfaceName);

        bool good();
        vector<unsigned char> read();
        void write(vector<unsigned char> data);

        static string asString(const vector<unsigned char>& data);
        static string asHexRepr(const vector<unsigned char>& data, bool doX = false, string delim = ", ");
        static vector<unsigned char> asHex(const string& data);
        static vector<unsigned char> slice(vector<unsigned char>& data, int i, int j=-1);
        static vector<unsigned char> concat(const vector<unsigned char>& v1, const vector<unsigned char>& v2);
};

ptrFctFwd( VRHDLC, vector<unsigned char> );

class VRHDLC {
    private:
        VRSerialPtr serial;
		VRHDLCCbPtr callback;
		bool verbose = 0;
		string serialState = "undefined";
		vector<unsigned char> serialData;
		vector<unsigned char> buffer;
		bool idle = true;
		size_t lastInput = 0;
		size_t pausingTimeS = 0;
		size_t pausingTimeR = 0;
		int pausingDurationS = 0;
		int pausingDurationR = 0;

    public:
        VRHDLC();
        ~VRHDLC();

        static VRHDLCPtr create();

        void setCallback(VRHDLCCbPtr cb);

        bool isIdle();
        string getInterface();

        void connect();
        bool connected();
        size_t getLastInput();
        void pauseSend(int T);
        void pauseReceive(int T);

        void handleData();
        bool handle(unsigned char c);
        bool process(vector<unsigned char> input);
        bool readData();
        void readAllData();

        void waitForMessage();
        void sendData(vector<unsigned char> data, bool doWait = true);

        void runTest();
};

OSG_END_NAMESPACE;

#endif // VRHDLC_H_INCLUDED
