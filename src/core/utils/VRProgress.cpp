#include "VRProgress.h"

VRProgress::VRProgress(string title, int max, Mode m) {
    this->title = title;
    N_100 = max/100.0;
    mode = m;
    reset();
}

VRProgress::~VRProgress() {
	if (mode == CONSOLE_M) cout << endl;
}

void VRProgress::setCallback(VRFunction<int>* cb) { callback = cb; }

void VRProgress::update(int i) {
    j+=i;
    if (j < N_100) return;

    j -= floor(N_100);
    k++;

    switch(mode) {
        case CONSOLE_M:
            cout << "\r" << title << " " << k << "%" << flush;
            break;
		case CALLBACK_M:
            (*callback)(k);
            break;
        case WIDGET_M:
            break;
    }
}

void VRProgress::reset() {
    k = j = 0;
}
