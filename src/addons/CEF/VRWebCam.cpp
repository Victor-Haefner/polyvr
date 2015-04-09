#include "VRWebCam.h"
#include "core/setup/VRSetup.h"
#include "core/setup/VRSetupManager.h"
#include "core/setup/devices/VRMobile.h"

using namespace OSG;

VRWebCam::VRWebCam(string name) : VRSprite(name) {}

void VRWebCam::connect(string uri, int res, float ratio) {
    this->uri = uri;
    site = "<html><body>";
    site += "<img src=\"" + uri + "\" alt=\"" + uri + "\"></img>";
    site += "</body></html>";

    VRMobile* mob = (VRMobile*)VRSetupManager::getCurrent()->getDevice("mobile");
    if (mob == 0) return;

    mob->addWebSite("internal_webcam_1234", site);

    webOpen("http://localhost:5500/internal_webcam_1234", res, ratio);
}
