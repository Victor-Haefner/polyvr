#ifndef VRGLUTWINDOW_H_INCLUDED
#define VRGLUTWINDOW_H_INCLUDED

#include "VRWindow.h"
#include <OpenSG/OSGGLUTWindow.h>

OSG_BEGIN_NAMESPACE;
using namespace std;


class VRGlutWindow : public VRWindow {
    private:
        GLUTWindowRecPtr win;
        int winID;

    public:
        VRGlutWindow();
        ~VRGlutWindow();

        void save(xmlpp::Element* node);
        void load(xmlpp::Element* node);
};

OSG_END_NAMESPACE;

#endif // VRGLUTWINDOW_H_INCLUDED
