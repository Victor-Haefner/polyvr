#ifndef VRSKIN_H_INCLUDED
#define VRSKIN_H_INCLUDED

#include <vector>
#include <OpenSG/OSGConfig.h>
#include "VRCharacterFwd.h"
#include "VRSkeleton.h"
#include "core/objects/VRObjectFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRSkin : public std::enable_shared_from_this<VRSkin> {
	private:
        VRSkeletonPtr skeleton;
        VRMaterialPtr material;
        vector<VRSkeleton::Bone> bone0s;
        vector<vector<pair<int, float>>> mapping;

        static string skinning_vp;
        static string skinning_fp;

	public:
		VRSkin(VRSkeletonPtr s);
		~VRSkin();

		static VRSkinPtr create(VRSkeletonPtr s);
		VRSkinPtr ptr();

		void addMap(int bID, float t, int vID = -1);
		void setMap(int bID, vector<float> t);
        void setMapping(vector<vector<pair<int, float>>> mapping);
        void applyMapping(VRGeometryPtr hull);
        size_t mapSize();

        void setDebugShader(bool b = true);

        void updateMappingTexture();
        void updateBoneTexture();

        VRMaterialPtr getMaterial();
};

OSG_END_NAMESPACE;

#endif //VRSKIN_H_INCLUDED
