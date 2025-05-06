#ifndef VRPDF_H_INCLUDED
#define VRPDF_H_INCLUDED

#include "core/objects/VRObjectFwd.h"
#include "core/math/VRMathFwd.h"
#include "core/utils/VRUtilsFwd.h"
#include "core/objects/geometry/drawing/VRDrawingFwd.h"

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGColor.h>
#include <string>

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRPDF {
    private:
        vector<VRPDFPagePtr> pages;

    public:
        VRPDF();
        ~VRPDF();

        static VRPDFPtr create();

        void read(string path);
        void write(string path);

        int getNPages();
        VRPDFPagePtr addPage();
        VRPDFPagePtr getPage(int i);
        void remPage(int i = -1);
};

OSG_END_NAMESPACE;

#endif // VRPDF_H_INCLUDED
