#ifndef VRPDF2DMODEL_H_INCLUDED
#define VRPDF2DMODEL_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/utils/VRUtilsFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRPDF2DModel : public std::enable_shared_from_this<VRPDF2DModel> {
	private:
	public:
		VRPDF2DModel();
		~VRPDF2DModel();

		static VRPDF2DModelPtr create();
		VRPDF2DModelPtr ptr();
};

OSG_END_NAMESPACE;

#endif //VRPDF2DMODEL_H_INCLUDED
