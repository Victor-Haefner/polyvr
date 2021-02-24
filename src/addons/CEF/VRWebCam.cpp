#include "VRWebCam.h"
#include "core/setup/VRSetup.h"
#include "core/setup/devices/VRServer.h"

using namespace OSG;

VRWebCam::VRWebCam(string name) : VRSprite(name) {}
VRWebCam::~VRWebCam() {}

shared_ptr<VRWebCam> VRWebCam::create(string name) { return shared_ptr<VRWebCam>(new VRWebCam(name)); }

void VRWebCam::connect(string uri, int res, float ratio) {
    this->uri = uri;
    site = "<html><body>";
    site += "<img src=\"" + uri + "\" alt=\"" + uri + "\"></img>";
    site += "</body></html>";

    VRServerPtr mob = dynamic_pointer_cast<VRServer>( VRSetup::getCurrent()->getDevice("server1") );
    if (!mob) return;

    mob->addWebSite("internal_webcam_1234", site);

    webOpen("http://localhost:5500/internal_webcam_1234", res, ratio);
}
