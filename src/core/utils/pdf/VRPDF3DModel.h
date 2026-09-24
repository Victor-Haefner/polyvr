#ifndef VRPDF3DMODEL_H_INCLUDED
#define VRPDF3DMODEL_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/objects/VRObjectFwd.h"
#include "core/utils/VRUtilsFwd.h"
#include "core/utils/pdf/VRPDFData.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRPDF3DModel : public std::enable_shared_from_this<VRPDF3DModel> {
	private:
	public:
		VRPDF3DModel();
		~VRPDF3DModel();

		static VRPDF3DModelPtr create();
		VRPDF3DModelPtr ptr();

        vector<VRTransformPtr> extract(vector<VRPDFData::Object>& objects, const std::vector<char>& buffer);
};

OSG_END_NAMESPACE;

#endif //VRPDF3DMODEL_H_INCLUDED
