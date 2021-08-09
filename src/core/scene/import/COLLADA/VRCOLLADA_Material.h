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
        VRSchedulerPtr scheduler;

        map<string, VRTexturePtr> library_images;
        map<string, string> sampler;
        map<string, string> surface;
        map<string, string> mappings;
        map<string, VRMaterialPtr> library_effects;
        map<string, VRMaterialPtr> library_materials;

        string filePath;
        string currentSampler;
        string currentSurface;
        string currentMaterial;
        string currentEffect;

	public:
		VRCOLLADA_Material();
		~VRCOLLADA_Material();

		static VRCOLLADA_MaterialPtr create();
		VRCOLLADA_MaterialPtr ptr();

		void finalize();

		void setFilePath(string fPath);
        void loadImage(string id, string path);
        void addSurface(string id);
        void addSampler(string id);
        void setSurfaceSource(string source);
        void setSamplerSource(string source);

        void newEffect(string id);
        void newMaterial(string id, string name);
        void closeEffect();
        void closeMaterial();
        void setColor(string sid, Color4f col, string eid = "");
        void setTexture(string sampler, string eid = "");
        void setShininess(float f, string eid = "");
        VRMaterialPtr getMaterial(string sid);
        void setMaterialEffect(string eid, string mid = "");
};

OSG_END_NAMESPACE;

#endif //VRCOLLADA_MATERIAL_H_INCLUDED
