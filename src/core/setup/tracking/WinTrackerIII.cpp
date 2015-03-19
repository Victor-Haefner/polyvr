#include <usb.h> /* libusb header */
#include <unistd.h> /* for geteuid */
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <map>
#include <termios.h>
#include <fcntl.h>
#include "WinTrackerIII.h"

//#define STANDALONE

using namespace std;

// --- IMPORTANT INFO ---
// lsusb -v -d 04b4:64df
//   will tell you all you need about the device

static const int vendorID=0x04b4;
static const int productID=0x64df;
static const int configuration=1; /*  Configuration 1*/
static const int interface=0;	/* Interface 0 */

static const int endpoint_in=1;
static const int endpoint_out=2;

static const int timeout=20000; /* timeout in ms */

/* max packet size is 64-bytes */
static const int reqLen=32;
typedef unsigned char byte;

struct usb_dev_handle * WG_fs_usb;
WTrackerSensor wtrackerSensors[N_SENSORS];

void bad(const char *why) {
	fprintf(stderr,"Fatal error: %s\n",why);
	exit(17);
}

/****************** Internal I/O Commands *****************/

/** Send this binary string command. */
static void send_usb(struct usb_dev_handle* handle, string s) {
    s += "\0";
    int r = usb_bulk_write(handle, endpoint_out, s.c_str(), s.size(), timeout);
    if( r < 0 ) {
        cout << "Error code: " << r << endl;
        perror("FS USB write");
        bad("USB write failed");
    }
}

/** Read this many bytes from this device */
static void recv_usb(struct usb_dev_handle* handle, int len, byte * dest) {
    int r = usb_bulk_read(handle, endpoint_in, (char*) dest, len, timeout);
    if( r != len ) {
        cout << "Error code: " << r << endl;
        perror("FS USB read");
        bad("USB read failed");
    }
}



/* debugging: enable debugging error messages in libusb */
//extern int usb_debug;

/* Find the first USB device with this vendor && product.
   Exits on errors, like if the device couldn't be found.
*/
struct usb_dev_handle *WG_fs_usb_open(void) {
    struct usb_device * device;
    struct usb_bus * bus;

    printf("Locating WinTracker (vendor 0x%04x/product 0x%04x)\n", vendorID, productID);
    /* (libusb setup code stolen from John Fremlin's cool "usb-robot") */

    // added the two debug lines
    usb_set_debug(255);
    printf("setting USB debug on by adding usb_set_debug(255) \n");
    //End of added codes

    usb_init();
    usb_find_busses();
    usb_find_devices();

    /* change by Xiaofan */
    /* libusb-win32: not using global variable like usb_busses*/
    /*  for (bus=usb_busses;bus!=NULL;bus=bus->next) */
    for (bus=usb_get_busses();bus!=NULL;bus=bus->next) {
        struct usb_device * usb_devices = bus->devices;
            for( device=usb_devices; device!=NULL; device=device->next ) {
            if( device->descriptor.idVendor == vendorID && device->descriptor.idProduct == productID ) {
               struct usb_dev_handle *d;
               printf("Found WinTracker as device '%s' on USB bus %s\n",
                       device->filename,
                       device->bus->dirname);
               d = usb_open(device);
               if( d ) { /* This is our device-- claim it */
                  if( usb_set_configuration(d, configuration) ) {
                     bad("Error setting USB configuration.\n");
                  }
                  if( usb_claim_interface(d, interface) ) {
                     bad("Claim failed-- the WinTracker is in use by another driver.\n");
                  }
                  printf("Communication established.\n");
                  return d;
               }
               else
                  bad("Open failed for USB device");
            }
        /* else some other vendor's device-- keep looking... */
        }
    }
    bad("Could not find WinTracker--\n"
      "you might try lsusb to see if it's actually there.");
    return NULL;
}

struct sensor_state {
    int ID;
    short int X,Y,Z;
    short int A,E,R;
    short int Qw,Qx,Qy,Qz;
    short int B;

    sensor_state(int id) {
        ID=id;
        X=Y=Z=0;
        A=E=R=0;
        Qw=Qx=Qy=Qz=0;
        B=0;
    }

    void print() {
        cout << "\nSensor " << ID << endl;
        cout << " x " << X << "  y " << Y << "  z " << Z << endl;
        cout << " a " << A << "  e " << E << "  r " << R << endl;
        cout << " qw " << Qw << "  qx " << Qx << "  qy " << Qy << "  qz " << Qz << endl;
    }
};

int kbhit(void) {
  struct termios oldt, newt;
  int ch;
  int oldf;

  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

  ch = getchar();

  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  fcntl(STDIN_FILENO, F_SETFL, oldf);

  if(ch != EOF) {
    ungetc(ch, stdin);
    return 1;
  }

  return 0;
}

#ifdef STANDALONE
int main(int argc, char ** argv) {
    cout << "\nStart wintracker\n";

    byte receive_buf[reqLen];
    byte* data;
    map<int, sensor_state*> sensors;

    WG_fs_usb = WG_fs_usb_open();
    //send_usb(WG_fs_usb, "SA111"); // enable all sensors
    send_usb(WG_fs_usb, "SC"); // enable all sensors

    //enable_cont_mode();
    while(!kbhit()) {
        //send_usb(WG_fs_usb, "SP"); /* Ask for sensor data */
        for(int i=0;i<32;i++) receive_buf[i]=0;
        recv_usb(WG_fs_usb, 32, receive_buf);
        if ( receive_buf[0] != 'D' && receive_buf[1] != 'D') continue;
        if ( receive_buf[0] == 'D') data = &receive_buf[0];
        else if( receive_buf[1] == 'D') data = &receive_buf[1];

        int ID = data[1] - '0';
        if(sensors.count(ID) == 0) sensors[ID] = new sensor_state(ID);

        sensor_state* s = sensors[ID];

        s->X = data[3]*256 + data[2];
        s->Y = data[5]*256 + data[4];
        s->Z = data[7]*256 + data[6];

        s->A = data[9]*256 + data[8];
        s->E = data[11]*256 + data[10];
        s->R = data[13]*256 + data[12];

        s->Qw = data[15]*256 + data[14];
        s->Qx = data[17]*256 + data[16];
        s->Qy = data[19]*256 + data[18];
        s->Qz = data[21]*256 + data[20];

        s->B = data[23]*256 + data[22];

        s->print();
    }


    printf("Shutting down wtracker\n");
    send_usb(WG_fs_usb, "Sc");
    usb_release_interface(WG_fs_usb, interface);
    usb_close(WG_fs_usb);
    return 0;
}
#endif


void enable_cont_mode() {
  /* Enable continous output mode */
  send_usb(WG_fs_usb, "SC");
  usleep(5000); //rkg - not sure if this is acutally needed
}

void disable_cont_mode() {
  /* Disable continous output mode (this has to be done when system parameters are changed)*/
  send_usb(WG_fs_usb, "Sc");
  usleep(5000);//rkg - not sure if this is acutally needed
}

short wtracker2short(char *buf) {
  short v;
  /* TODO - this does not work on smallendian systems */
  ((char*)&v)[0]=buf[0];
  ((char*)&v)[1]=buf[1];
  return v;
}

void setFrontHemisphere() {
  send_usb(WG_fs_usb, "SHF");
  usleep(5000);
}
void setUpHemisphere() {
  send_usb(WG_fs_usb, "SHU");
  usleep(5000);
}


void tick_wtracker() {
  char receive_buf[40], *sensorData;
  //  int intA;
  int sensor;
  int ret,i;

  /* Ask for sensor data */
  send_usb(WG_fs_usb, "SP");

  for(i=0;i<32;i++) receive_buf[i]=0;
  ret=usb_bulk_read(WG_fs_usb, endpoint_in, (char*)&receive_buf[0], 32, 50); /* wait up to 50ms for the data */
  /*printf("%d bytes: ",ret);
  for(i=0;i<32;i++) printf("%02x ",receive_buf[i]);
  printf("\n");*/
  if(ret != 32) return;

  if(receive_buf[0] == 'D') sensorData = &receive_buf[0];
  else if(receive_buf[1] == 'D') sensorData = &receive_buf[1];
  else return;

  sensor = sensorData[1] - '0';
  /*printf("Got sensor %d ('%c')\n",sensor,sensorData[1]);*/


  if(sensor >= 0 && sensor < N_SENSORS) {

    /*if(sensor == 0) {
      printf("Received buf: "); for(i=0;i<32;i++) printf("%02x ",receive_buf[i]);
      printf("\n");
      }*/

    wtrackerSensors[sensor].x = wtracker2short(&sensorData[2]) * 0.0001;
    wtrackerSensors[sensor].y = wtracker2short(&sensorData[4]) * 0.0001;
    wtrackerSensors[sensor].z = wtracker2short(&sensorData[6]) * 0.0001;

    wtrackerSensors[sensor].a = wtracker2short(&sensorData[8]) * 0.01;
    wtrackerSensors[sensor].e = wtracker2short(&sensorData[10]) * 0.01;
    wtrackerSensors[sensor].r = wtracker2short(&sensorData[12]) * 0.01;

    wtrackerSensors[sensor].qw = wtracker2short(&sensorData[14]) * 0.01;
    wtrackerSensors[sensor].qx = wtracker2short(&sensorData[16]) * 0.01;
    wtrackerSensors[sensor].qy = wtracker2short(&sensorData[18]) * 0.01;
    wtrackerSensors[sensor].qz = wtracker2short(&sensorData[20]) * 0.01;

    wtrackerSensors[sensor].button = wtracker2short(&sensorData[20]);

      /*printf("sensors[%d].xyz = %3.3f %3.3f %3.3f\n",sensor,wtrackerSensors[sensor].x,wtrackerSensors[sensor].y,wtrackerSensors[sensor].z);*/

    } else printf("Received invalid sensor: %d\n",sensor);
}


