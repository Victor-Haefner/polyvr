#ifndef VRMILLINGCUTTINGTOOLPROFILE_H
#define VRMILLINGCUTTINGTOOLPROFILE_H

#include <string>
#include <vector>
#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGGeoProperties.h>

OSG_BEGIN_NAMESPACE;

using namespace std;

class VRMillingCuttingToolProfile
{
    public:
        VRMillingCuttingToolProfile();
        virtual ~VRMillingCuttingToolProfile();
        void addPointProfile(Vec2f point);
        float maxProfile(Vec3f position, Vec3f cubePosition, Vec3f cubeSize);
        float getLength();

    protected:
        bool alreadyInProfile(float newx);
        float newy(float newx, int p);
        int lookForNearestIndex(float newx);
        float lookForMaxInList(vector<Vec2f> liste);

    private:
        vector<Vec2f> profile;
};

OSG_END_NAMESPACE;

#endif // VRMILLINGCUTTINGTOOLPROFILE_H
