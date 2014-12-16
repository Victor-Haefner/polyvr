#include "VRRecorder.h"
#include "core/setup/windows/VRView.h"
#include "core/setup/VRSetup.h"
#include "core/setup/VRSetupManager.h"
#include "core/objects/object/VRObject.h"

#include <OpenSG/OSGImage.h>
#include <GL/glut.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRFrame {
    public:
        ImageRecPtr capture = 0;
        int timestamp = 0;
};

VRRecorder::VRRecorder() {;}

void VRRecorder::setView(int i) {
    view = VRSetupManager::getCurrent()->getView(i);
}

void VRRecorder::capture() {
    if (view == 0) return;
    int ts = VRGlobals::get()->CURRENT_FRAME;
    captures[ts] = new VRFrame();
    captures[ts]->capture = view->grab();
    captures[ts]->timestamp = glutGet(GLUT_ELAPSED_TIME);
}

void VRRecorder::clear() {
    for (auto f : captures) delete f.second;
    captures.clear();
}

int VRRecorder::getRecordingSize() { return captures.size(); }
float VRRecorder::getRecordingLength() {
    if (captures.size() == 0) return 0;
    int t0 = captures.begin()->second->timestamp;
    int t1 = captures.rbegin()->second->timestamp;
    return (t1-t0)*0.001; //seconds
}

void VRRecorder::compile(string path) {
    for (auto f : captures) {
        if (f.second->capture) f.second->capture->write("bla.png");
    }
}

OSG_END_NAMESPACE;
