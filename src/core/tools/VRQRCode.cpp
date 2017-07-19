#include "VRQRCode.h"
#include <qrencode.h>
#include <OpenSG/OSGImage.h>
#include "core/objects/material/VRMaterial.h"
#include "core/objects/material/VRTexture.h"

void createQRCode(std::string s, OSG::VRMaterialPtr mat, OSG::Vec3d fg, OSG::Vec3d bg, int offset) {
    OSG::ImageRecPtr img = OSG::Image::create();

    QRcode* code = QRcode_encodeString(s.c_str(), 0, QR_ECLEVEL_H, QR_MODE_8, 1);
    if (code == NULL) { cout << "\nQR code failed\n"; return; }

    int w = code->width + 2*offset;
	vector<OSG::Vec3d> data;
	data.resize(w*w);

    for (int i=0; i<w; i++) {
        for (int j=0; j<w; j++) {
            int k = i+j*w;
            if (i<offset || i>=w-offset || j<offset || j>=w-offset) { data[k] = bg; continue; }

            unsigned char q = code->data[(i-offset) + (j-offset)*code->width];
            data[k] = q & 1 ? fg : bg;
        }
    }

    img->set(OSG::Image::OSG_RGB_PF, w, w, 1, 0, 1, 0.0, (const uint8_t*)&data[0], OSG::Image::OSG_FLOAT32_IMAGEDATA, true, 1);

    mat->setTexture( OSG::VRTexture::create(img) );
}

