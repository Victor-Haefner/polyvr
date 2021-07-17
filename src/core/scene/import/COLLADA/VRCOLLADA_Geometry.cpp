#include "VRCOLLADA_Geometry.h"

using namespace OSG;

VRCOLLADA_Geometry::VRCOLLADA_Geometry() {}
VRCOLLADA_Geometry::~VRCOLLADA_Geometry() {}

VRCOLLADA_GeometryPtr VRCOLLADA_Geometry::create() { return VRCOLLADA_GeometryPtr( new VRCOLLADA_Geometry() ); }
VRCOLLADA_GeometryPtr VRCOLLADA_Geometry::ptr() { return static_pointer_cast<VRCOLLADA_Geometry>(shared_from_this()); }
