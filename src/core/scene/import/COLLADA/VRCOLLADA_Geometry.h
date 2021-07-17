#ifndef VRCOLLADA_GEOMETRY_H_INCLUDED
#define VRCOLLADA_GEOMETRY_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/scene/import/VRImportFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRCOLLADA_Geometry : public std::enable_shared_from_this<VRCOLLADA_Geometry> {
	private:
	public:
		VRCOLLADA_Geometry();
		~VRCOLLADA_Geometry();

		static VRCOLLADA_GeometryPtr create();
		VRCOLLADA_GeometryPtr ptr();
};

OSG_END_NAMESPACE;

#endif //VRCOLLADA_GEOMETRY_H_INCLUDED
