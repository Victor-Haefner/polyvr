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
        map<string, VRTexturePtr> library_images;
        map<string, string> sampler;
        map<string, string> surface;
        map<string, VRMaterialPtr> library_effects;
        map<string, VRMaterialPtr> library_materials;
        VRMaterialPtr currentEffect;
        VRMaterialPtr currentMaterial;

        string filePath;
        string currentSampler;
        string currentSurface;

	public:
		VRCOLLADA_Material();
		~VRCOLLADA_Material();

		static VRCOLLADA_MaterialPtr create();
		VRCOLLADA_MaterialPtr ptr();

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
        void setColor(string sid, Color4f col);
        void setTexture(string sampler);
        void setShininess(float f);
        VRMaterialPtr getMaterial(string sid);
        bool setMaterialEffect(string eid);
};

OSG_END_NAMESPACE;

#endif //VRCOLLADA_MATERIAL_H_INCLUDED
