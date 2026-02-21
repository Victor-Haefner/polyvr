#ifndef VRGLUTSIGNALS_H_INCLUDED
#define VRGLUTSIGNALS_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/setup/VRSetupFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class GlutSignals : public std::enable_shared_from_this<GlutSignals> {
	private:
	public:
		GlutSignals();
		~GlutSignals();

		static GlutSignalsPtr create();
		GlutSignalsPtr ptr();
};

OSG_END_NAMESPACE;

#endif //VRGLUTSIGNALS_H_INCLUDED
