#include "BaseModule.h"

#include <boost/format.hpp>

using namespace std;
using namespace realworld;


AreaBoundingBox::AreaBoundingBox(OSG::Vec2f min, float gridSize) {
    this->min = min;
    this->max = min + OSG::Vec2f(gridSize, gridSize);
    this->str = (boost::format("%.3f") % (round(min.getValues()[0]*1000) / 1000)).str() + "-" +
                (boost::format("%.3f") % (round(min.getValues()[1]*1000) / 1000)).str();
}

VRObject* BaseModule::getRoot() { return root; }

BaseModule::BaseModule(MapCoordinator* mapCoordinator, TextureManager* texManager) {
    this->mapCoordinator = mapCoordinator;
    this->texManager = texManager;
    root = new VRObject("ModuleRoot");
    root->setPersistency(0);
}
