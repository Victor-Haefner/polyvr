#include "VRProgress.h"
#include "VRFunction.h"
#include "toString.h"
#include "VRTimer.h"

using namespace OSG;


VRProgress::VRProgress(string title, size_t max, Mode m) {
    timer = VRTimer::create();
    setup(title, max, m);
}

VRProgress::~VRProgress() {
	if (mode == CONSOLE_M) cout << endl;
}

void VRProgress::setCallback(VRAnimCbPtr cb) { callback = cb; }

VRProgressPtr VRProgress::create(string title, size_t max, Mode m) { return VRProgressPtr( new VRProgress(title, max, m) ); }

void VRProgress::update(size_t i) {
    if (count < max) {
        count += i;
        double k = double(count)/max;
        if (k-part < 0.01) return;
        part = k;
    }

    double dt = timer->stop()*0.001;
    if (count == 0) return;
    double pending = dt*(max/double(count)-1);

    switch(mode) {
        case CONSOLE_M:
            cout << "\r" << title << " " << this << " " << long(part*100) << "% - pending " << pending << " s, " << pending/60.0 << " min" << flush;
            break;
		case CALLBACK_M:
            if (auto cl = callback.lock()) (*cl)(part*100);
            break;
        case WIDGET_M:
            break;
        case GEOM_M:
            break;
    }
}

void VRProgress::finish() { count = max; part = 1.0; update(0); }
float VRProgress::get() { return part; }
void VRProgress::set(float t) { part = t; }
void VRProgress::reset() { part = count = 0; timer->reset(); }

void VRProgress::setup(string title, size_t max, Mode m) {
    this->title = title;
    this->max = max;
    mode = m;
    reset();
}

template<> int toValue(stringstream& ss, VRProgress::Mode& m) {
    int M;
    int res = toValue<int>(ss, M);
    m = (VRProgress::Mode)M;
    return res;
}
