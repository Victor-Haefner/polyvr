#include "VRPDF2DModel.h"

using namespace OSG;

VRPDF2DModel::VRPDF2DModel() {}
VRPDF2DModel::~VRPDF2DModel() {}

VRPDF2DModelPtr VRPDF2DModel::create() { return VRPDF2DModelPtr( new VRPDF2DModel() ); }
VRPDF2DModelPtr VRPDF2DModel::ptr() { return static_pointer_cast<VRPDF2DModel>(shared_from_this()); }
