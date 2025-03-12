#ifndef VRPLAYER_H_INCLUDED
#define VRPLAYER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "VRToolsFwd.h"
#include "core/utils/VRFunctionFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRPlayer : public std::enable_shared_from_this<VRPlayer> {
	private:
	    VRAnimCbPtr callback;
	    VRUpdateCbPtr updateCb;
	    double progress;
	    double speed;

	    double lastUpdateTime = 0;

	    void update();

	public:
		VRPlayer();
		~VRPlayer();

		static VRPlayerPtr create();
		VRPlayerPtr ptr();

		void setCallback(VRAnimCbPtr cb);

		void reset();
		void pause();
		void play(double speed);
		void moveTo(double progress);
};

OSG_END_NAMESPACE;

#endif //VRPLAYER_H_INCLUDED
