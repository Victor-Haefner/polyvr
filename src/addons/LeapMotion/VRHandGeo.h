#pragma once

#include "core/objects/geometry/VRGeometry.h"
#include "VRLeap.h"

#include <boost/thread/mutex.hpp>

OSG_BEGIN_NAMESPACE ;


class VRHandGeo : public VRGeometry {

public:

    static VRHandGeoPtr create(string name = "HandGeo");

    void update(VRLeapFramePtr frame);

    void setLeft();

    void setRight();

    void connectToLeap(VRLeapPtr leap);

private:

    VRHandGeo(string name);

    void updateHandGeo();

    VRUpdateCbPtr updateCb;

    boost::mutex mutex;
    bool isLeft{false};
    bool initialized{false};
    bool visible{false};

    vector<vector<VRGeometryPtr>> bones;
    VRGeometryPtr pinch;
    VRLeapFrame::HandPtr handData;
};

OSG_END_NAMESPACE;
