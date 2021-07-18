#ifndef VRCOLLADA_MATERIAL_H_INCLUDED
#define VRCOLLADA_MATERIAL_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/scene/import/VRImportFwd.h"
#include "core/objects/material/VRMaterial.h"

#include <map>

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRCOLLADA_Material : public std::enable_shared_from_this<VRCOLLADA_Material> {
	private:
        map<string, VRMaterialPtr> library_effects;
        VRMaterialPtr currentMaterial;

	public:
		VRCOLLADA_Material();
		~VRCOLLADA_Material();

		static VRCOLLADA_MaterialPtr create();
		VRCOLLADA_MaterialPtr ptr();

        void newEffect(string id);
        void closeEffect();
        void setColor(string sid, Color4f col);
};

OSG_END_NAMESPACE;

#endif //VRCOLLADA_MATERIAL_H_INCLUDED
