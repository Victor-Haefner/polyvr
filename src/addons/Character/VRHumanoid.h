#ifndef VRHUMANOID_H_INCLUDED
#define VRHUMANOID_H_INCLUDED

#include "core/objects/geometry/VRGeometry.h"
#include "VRCharacterFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRHumanoid : public VRGeometry {
	private:
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
};

OSG_END_NAMESPACE;

#endif //VRHUMANOID_H_INCLUDED
