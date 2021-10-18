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
        map<int, vector<Vec3d>> ringParams;

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

        Color3f getColor(string pID);
        vector<Vec3d> getRingParams(int rID);
        string getParameterString();

        void setColor(string pID, Color3f c);
        void setRingParams(int rID, vector<double> params);
};

OSG_END_NAMESPACE;

#endif //VRHUMANOID_H_INCLUDED
