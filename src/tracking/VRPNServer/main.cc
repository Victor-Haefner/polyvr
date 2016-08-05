#ifndef MAIN_CC_INCLUDED
#define MAIN_CC_INCLUDED

#include <stdlib.h>
#include <string>
#include <iostream>
#include <unistd.h>

#include <vrpn/vrpn_Connection.h>
#include <vrpn/vrpn_Button.h>
#include <vrpn/vrpn_Analog.h>
#include <vrpn/vrpn_Tracker.h>

using namespace std;

class Button : public vrpn_Button_Filter {
    private:
    public:
        Button(string id, vrpn_Connection* c) : vrpn_Button_Filter(id.c_str(), c) {}
        void mainloop() {}
};

class Tracker : public vrpn_Tracker {
    private:
    public:
        Tracker(string id, vrpn_Connection* c) : vrpn_Tracker(id.c_str(), c) {}
        void mainloop() {}
};

class Slider : public vrpn_Analog {
    private:
    public:
        Slider(string id, vrpn_Connection* c) : vrpn_Analog(id.c_str(), c) {}
        void mainloop() {}
};

class Device : public Button, public Slider, public Tracker {
    private:
        struct timeval timestamp;

        void fetch_values() {
            for( int i=0; i<Button::num_buttons; i++) Button::buttons[i] = rand()%2;
            for( int i=0; i<Slider::num_channel; i++) Slider::channel[i] = (rand()%10)*0.1;
            //for( int i=0; i<Tracker::num_sensors; i++) Tracker::sensor[i] = (rand()%10)*0.1;
            //Tracker::
            vrpn_gettimeofday( &timestamp, NULL );
        }

        void report_changes() {
            Button::timestamp = timestamp;
            Button::report_changes();
            Slider::timestamp = timestamp;
            Slider::report_changes( vrpn_CONNECTION_LOW_LATENCY );
        }

    public:
        Device(string name, int Nb, int Nj, int Nt, vrpn_Connection* c) : Button(name, c), Slider(name, c), Tracker(name, c) {
            Button::num_buttons = Nb;
            Slider::num_channel = Nj;
            Tracker::num_sensors = Nt;

            for( int i=0; i<Nb; i++) Button::buttons[i] = Button::lastbuttons[i] = 0;
            for( int i=0; i<Nj; i++) Slider::channel[i] = Slider::last[i] = 0;
        }

        ~Device() {}

        void mainloop() {
            fetch_values();
            report_changes();
            server_mainloop();
        }
};

int main (int argc, char *argv[]) {
    int port = vrpn_DEFAULT_LISTEN_PORT_NO; // default port
    string name = "myDevice";
    if (argc > 1) port = atoi(argv[1]);
    if (argc > 2) name = argv[2];
    cout << "start VRPN server on port " << port << " with name " << name << endl;

    //vrpn_Connection* connection = new vrpn_Connection_IP(port);
    vrpn_Connection* connection = vrpn_create_server_connection(port);
    Device dev(name,3,2,1,connection);

    while (true) {
        dev.mainloop();
        connection->mainloop(); // Send and receive all messages
        usleep(1000);
    }

    delete connection;
}

#endif // MAIN_CC_INCLUDED
