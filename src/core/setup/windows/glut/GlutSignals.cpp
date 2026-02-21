#include "GlutSignals.h"

using namespace OSG;

GlutSignals::GlutSignals() {}
GlutSignals::~GlutSignals() {}

GlutSignalsPtr GlutSignals::create() { return GlutSignalsPtr( new GlutSignals() ); }
GlutSignalsPtr GlutSignals::ptr() { return static_pointer_cast<GlutSignals>(shared_from_this()); }
