#include "BaseModule.h"
#include "core/objects/object/VRObject.h"
#include <boost/format.hpp>

using namespace OSG;

AreaBoundingBox::AreaBoundingBox(Vec2f min, float gridSize) {
    this->min = min;
    this->max = min + Vec2f(gridSize, gridSize);
    this->str = (boost::format("%.3f") % (round(min.getValues()[0]*1000) / 1000)).str() + "-" +
                (boost::format("%.3f") % (round(min.getValues()[1]*1000) / 1000)).str();
}

BaseModule::BaseModule(string name) {
    this->name = name;
    root = VRObject::create(name+"Root");
    root->setPersistency(0);
}

VRObjectPtr BaseModule::getRoot() { return root; }
string BaseModule::getName() { return name; }


