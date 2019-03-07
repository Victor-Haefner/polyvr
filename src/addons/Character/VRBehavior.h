#ifndef VRBEHAVIOR_H_INCLUDED
#define VRBEHAVIOR_H_INCLUDED

#include "core/utils/VRName.h"
#include "VRCharacterFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRBehavior : public VRName {
    public:
        /*struct Action : public VRName {
            map<string,VRSkeleton::ConfigurationPtr> configurations;

            Action(string name);
            ~Action();

            static shared_ptr<Action> create(string name);

            void addConfiguration(VRSkeleton::ConfigurationPtr c);
        };

        typedef shared_ptr<Action> ActionPtr;*/

    private:
        VRSkeletonPtr skeleton;
        //map<string, ActionPtr> actions; // perhaps a state machine??
        //VRSkeleton::ConfigurationPtr current;
        VRAnimationPtr animation;

        void updateBones();

    public:
        VRBehavior(string name);
        ~VRBehavior();

        static VRBehaviorPtr create(string name = "bahavior");

        void setSkeleton(VRSkeletonPtr s);
};

OSG_END_NAMESPACE;


#endif // VRBEHAVIOR_H_INCLUDED
