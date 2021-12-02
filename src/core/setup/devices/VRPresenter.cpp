#include "VRPresenter.h"
#include "core/utils/VRFunction.h"
#include "core/scene/VRSceneManager.h"

#include <fcntl.h>
#include <linux/input.h>

using namespace OSG;


VRPresenter::VRPresenter() : VRFlystick() {
    input = "/dev/input/presenter";

    store("input", &input);

    regStorageSetupFkt( VRStorageCb::create("connect device", bind(&VRPresenter::setup, this, placeholders::_1)) );
}

VRPresenter::~VRPresenter() {
    disconnectDevice();
}

VRPresenterPtr VRPresenter::create() {
    auto m = VRPresenterPtr(new VRPresenter());
    m->initIntersect(m);
    m->clearSignals();
    return m;
}

VRPresenterPtr VRPresenter::ptr() { return static_pointer_cast<VRPresenter>( shared_from_this() ); }

void VRPresenter::setup(VRStorageContextPtr context) {
    connectDevice();
}

string VRPresenter::getDevice() { return input; }

void VRPresenter::setDevice(string devName) {
    disconnectDevice();
    input = devName;
    connectDevice();
}

void VRPresenter::updateDevice() {
    struct input_event ev;
    static int oldButtonState = 0;
    int ret = 0;

    vector<int>   buttons = {0,0,0,0};
    vector<float> sliders = {0,0};

    while ((ret = read(fd, &ev, sizeof(ev))) == sizeof(ev)) {
	    if (ev.type == EV_KEY) {
            int b = -1;
            if (ev.code == 104)                                             b = 0;// left
            else if (ev.code == 109)                                        b = 1; // right
            else if ((ev.code == 1) || (ev.code == 63) || ev.code == 425)   b = 2; // esc/F5
            else if (ev.code == 52 || ev.code == 431)                       b = 3; // blank
            else if (ev.code == 114)                                        b = 4; // -
            else if (ev.code == 115)                                        b = 5; // +
            else if (ev.code >= BTN_0 && ev.code <= BTN_9)                  b = ev.code - BTN_0;
            else if (ev.code >= BTN_TRIGGER && ev.code <= BTN_DEAD)         b = ev.code - BTN_TRIGGER;
            else if (ev.code >= BTN_A && ev.code <= BTN_THUMBR)             b = ev.code - BTN_A;
            else if (ev.code >= BTN_LEFT && ev.code <= BTN_TASK)            b = ev.code - BTN_LEFT;
            else std::cerr << "unknown evdev code: " << ev.code << std::endl;

            if (ev.value > 0) oldButtonState |= 1 << b;
            else oldButtonState &= ~(1 << b);
            bool pressed = (oldButtonState>0);

	        cout << " VRPresenter::updateDevice key: " << b << ", state: " << pressed << endl;
            if (b == 1) buttons[0] = pressed;
            if (b == 0) sliders[1] = 1.0 * pressed;
            if (b == 2) sliders[0] = -1.0 * pressed;
            if (b == 3) sliders[0] =  1.0 * pressed;
            VRFlystick::update(buttons);
            VRFlystick::update(sliders);
	    }
    }
}

void VRPresenter::disconnectDevice() {
    close(fd);
}

void VRPresenter::connectDevice() {
    cout << "VRPresenter::connectDevice" << endl;
    fd = open(input.c_str(), O_RDONLY | O_NONBLOCK);
    if (fd < 0) { cout << "VRPresenter::connectDevice Error: could not open device " << input << endl; return; }

    updatePtr = VRUpdateCb::create( "Presenter_update", bind(&VRPresenter::updateDevice, this) );
    VRSceneManager::get()->addUpdateFkt(updatePtr);
    cout << "VRPresenter::connectDevice successfully connected to device " << input << endl;
}


