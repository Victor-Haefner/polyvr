#ifndef VRHUMANOID_H_INCLUDED
#define VRHUMANOID_H_INCLUDED

#include "core/objects/geometry/VRGeometry.h"
#include "VRCharacterFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRHumanoid : public VRGeometry {
	private:
        VRSkeletonPtr skeleton;
        VRSkinPtr skin;

        map<string, Color3f> colors;

        void generateTorso(VRGeoData& data);
        void generateLegs(VRGeoData& data);
        void generateArms(VRGeoData& data);
        void generateHead(VRGeoData& data);

        void generate();

	public:
		VRHumanoid(string name);
		~VRHumanoid();

		static VRHumanoidPtr create(string name = "Jane");
		VRHumanoidPtr ptr();

        VRSkinPtr getSkin();
        VRSkeletonPtr getSkeleton();

        void setColor(string pID, Color3f c);
};

OSG_END_NAMESPACE;

#endif //VRHUMANOID_H_INCLUDED
