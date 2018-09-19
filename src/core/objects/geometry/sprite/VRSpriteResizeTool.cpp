#include "VRSpriteResizeTool.h"

using namespace OSG;

VRSpriteResizeTool::VRSpriteResizeTool(VRSpritePtr t) : VRObject("resizeTool") { target = t; }
VRSpriteResizeTool::~VRSpriteResizeTool() {}

VRSpriteResizeToolPtr VRSpriteResizeTool::create(VRSpritePtr t) { return VRSpriteResizeToolPtr( new VRSpriteResizeTool(t) ); }
