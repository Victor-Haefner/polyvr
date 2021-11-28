#ifndef VRMICROPHONE_H_INCLUDED
#define VRMICROPHONE_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "VRSoundFwd.h"

struct ALCdevice_struct;
namespace std { struct thread; }

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRMicrophone : public std::enable_shared_from_this<VRMicrophone> {
	private:
        int sample_size = 1024;
        int sample_rate = 22050;
        bool doRecord = false;

        thread* recordingThread = 0;
	    ALCdevice_struct* device = 0;
	    VRSoundPtr recording;

	    VRSoundBufferPtr frame;

	    void setup();

	public:
		VRMicrophone();
		~VRMicrophone();

		static VRMicrophonePtr create();
		VRMicrophonePtr ptr();

		void startRecording();
		VRSoundPtr stopRecording();
};

OSG_END_NAMESPACE;

#endif //VRMICROPHONE_H_INCLUDED
