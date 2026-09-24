#ifndef VRPDFPAGE_H_INCLUDED
#define VRPDFPAGE_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGColor.h>
#include "core/objects/VRObjectFwd.h"
#include "core/objects/geometry/drawing/VRDrawingFwd.h"
#include "core/utils/VRUtilsFwd.h"
#include "core/math/VRMathFwd.h"

struct _cairo;

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRPDFPage : public std::enable_shared_from_this<VRPDFPage> {
	private:
	    VRPDFDataPtr data;

        int res = 300;
        float W = 100;
        float H = 100;
        float Ox = 100;
        float Oy = 100;

	public:
		VRPDFPage(VRPDFDataPtr data);
		~VRPDFPage();

		static VRPDFPagePtr create(VRPDFDataPtr data);
		VRPDFPagePtr ptr();

		void writeTo(_cairo* cr);

        void drawLine(Pnt2d p1, Pnt2d p2, Color3f c1, Color3f c2);
        void drawText();

        void project(VRObjectPtr obj, PosePtr plane);
        void slice(VRObjectPtr obj, PosePtr plane);

        // file handling
        vector<VRTechnicalDrawingPtr> extract2DModels();
        vector<VRTransformPtr> extract3DModels();
};

OSG_END_NAMESPACE;

#endif //VRPDFPAGE_H_INCLUDED
