#pragma once

#include "core/objects/geometry/VRGeometry.h"
#include "VRLeap.h"

#ifndef __EMSCRIPTEN__
#include <boost/thread/mutex.hpp>
#endif

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

#ifndef __EMSCRIPTEN__
    boost::mutex mutex;
#endif
    bool isLeft{false};
    bool initialized{false};
    bool visible{false};

    vector<vector<VRGeometryPtr>> bones;
    VRGeometryPtr pinch;
    VRLeapFrame::HandPtr handData;
};

OSG_END_NAMESPACE;
