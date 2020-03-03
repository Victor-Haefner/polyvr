#pragma once

#include "core/math/pose.h"
#include "core/utils/VRDeviceFwd.h"

OSG_BEGIN_NAMESPACE;

class VRLeapFrame : public std::enable_shared_from_this<VRLeapFrame> {
    public:
        ptrFwd(Hand);
        ptrFwd(Pen);

        struct Pen {
            PosePtr pose = 0;
            float length;
            float width;

            Pen();
            ~Pen();

            void transform(Pose transformation);
        };

        struct Hand {
            PosePtr pose = 0; // pos, dir, normal
            vector<vector<Vec3d>> joints; // joint positions of each finger, 0 is thumb -> 4 is pinky
            vector<vector<Pose>> bases; // 3 basis vectors for each bone, in index order, wrist to tip
            vector<bool> extended; // True, if the finger is a pointing, or extended, posture
            float pinchStrength;
            float grabStrength;
            float confidence; //"How well the internal hand model fits the observed data." - see Leap Documentation
            bool isPinching = 0;

            Hand() : pose(Pose::create()), joints(5), bases(5), extended(5) { }
            HandPtr clone();

            void transform(Pose transformation);
        };

    private:
        VRLeapFrame() {}

        HandPtr rightHand;
        HandPtr leftHand;
        vector<PenPtr> pens;

    public:
        static VRLeapFramePtr create();
        VRLeapFramePtr ptr();

        HandPtr getLeftHand();
        HandPtr getRightHand();

        void setLeftHand(HandPtr hand);
        void setRightHand(HandPtr hand);

        void insertPen(PenPtr pen);
        vector<PenPtr> getPens();
};

typedef VRLeapFrame::HandPtr HandPtr;
typedef VRLeapFrame::PenPtr PenPtr;

OSG_END_NAMESPACE;

