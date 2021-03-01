#include "VRMillingMachine.h"
#include "core/objects/VRTransform.h"
#include "core/networking/VRSocket.h"
#include "core/utils/toString.h"
#ifndef WITHOUT_CURL
#include <curl/curl.h>
#endif

using namespace OSG;

VRMillingMachine::VRMillingMachine() {
    pos = new Vec3d();
    http = new VRSocket("milling machine");
}

VRMillingMachine::~VRMillingMachine() {
    delete pos;
    delete http;
}

shared_ptr<VRMillingMachine> VRMillingMachine::create() { return shared_ptr<VRMillingMachine>(new VRMillingMachine()); }

int VRMillingMachine::getState() { return state; }
int VRMillingMachine::getMode() { return mode; }

bool VRMillingMachine::connected() { return online; }

void VRMillingMachine::connect(string s) {
    address = s;
    online = true;
}

void VRMillingMachine::disconnect() {
    online = false;
}

void VRMillingMachine::setSpeed(float s) { speed = s; }

void VRMillingMachine::setPosition(Vec3d p) {
    if (online) return; // position comes from machine
    *pos = p*1000;
}

void VRMillingMachine::setSpeed(Vec3d v) {
    //Vec3d t = Vec3d(-v[1], v[2], -v[0]);
    Vec3d t = Vec3d(-v[2], -v[0], v[1]);
    v = t*1000;

    float vmin = 0.3;
    if (online && state == 4 && mode == 1) {
        if (abs(v[0]) > vmin) post("c","jog(1,0,"+toString(v[0])+")");
        else post("c","jog(0,0)");
        if (abs(v[1]) > vmin) post("c","jog(1,1,"+toString(v[1])+")");
        else post("c","jog(0,1)");
        if (abs(v[2]) > vmin) post("c","jog(1,2,"+toString(v[2])+")");
        else post("c","jog(0,2)");
    }
}

void VRMillingMachine::setGeometry(vector<VRTransformPtr> geos) { this->geos = geos; }

void VRMillingMachine::update() {
	if (online) {
		string re = post("p"," ");
		vector<string> res;
		stringstream ss(re);
		while (getline(ss, re, '%')) res.push_back(re); // split by '%'

		*pos = Vec3d( toFloat(res[0]), toFloat(res[1]), toFloat(res[2]) );
		state = toInt(res[5]);
		mode = toInt(res[6]);
	} else {
		state = 1;
		mode = 1;
	}

	Vec3d p = *pos;
	float f = 0.001;
	geos[0]->setFrom( Vec3d( 0.000,0.250+(p[0]*f),0.033));
	geos[0]->setAt(   Vec3d( 0.000,-1.00+(p[0]*f),0.033));
	geos[1]->setFrom( Vec3d( 0.031+(p[1]*f),0.230,0.181));
	geos[1]->setAt(   Vec3d( 0.031+(p[1]*f),-1.00,0.181));
	geos[2]->setFrom( Vec3d( -0.069+(p[1]*f),0.268,0.181+(p[2]*f)));
	geos[2]->setAt(   Vec3d( -0.069+(p[1]*f),-1.00,0.181+(p[2]*f)));
}

Vec3d VRMillingMachine::getPosition() { return *pos; }

size_t httpwritefkt( char *ptr, size_t size, size_t nmemb, void *userdata) {
    string* s = (string*)userdata;
    s->append(ptr, size*nmemb);
    return size*nmemb;
}

string VRMillingMachine::post(string cmd, string data) {
#ifndef WITHOUT_CURL
    if (cmd == "c") cout << "POST " << data << endl;

    auto curl = curl_easy_init();

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-type: text/plain");
    headers = curl_slist_append(headers, ("Content-length: "+toString(data.size())).c_str());
    headers = curl_slist_append(headers, ("CNC: "+cmd).c_str());

    //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
    curl_easy_setopt(curl, CURLOPT_POST, 1);
    curl_easy_setopt(curl, CURLOPT_URL, address.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, data.size());

    string res;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, httpwritefkt);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA , &res);
    CURLcode r = curl_easy_perform(curl);
    if(r != CURLE_OK) fprintf(stderr, "\ncurl_easy_perform() failed: %s\n", curl_easy_strerror(r));
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return res;
#endif
	return "";
}
