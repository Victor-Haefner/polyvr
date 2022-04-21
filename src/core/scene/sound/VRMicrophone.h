#ifndef VRMICROPHONE_H_INCLUDED
#define VRMICROPHONE_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <list>
#include <string>

#include "VRSoundFwd.h"
#include "core/networking/VRNetworkingFwd.h"

#ifdef _WIN32
struct ALCdevice;
#else
struct ALCdevice_struct;
#endif

namespace std { struct thread; }

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRMutex;

class VRMicrophone : public std::enable_shared_from_this<VRMicrophone> {
	private:
        int sample_size = 1024;
        int sample_rate = 22050;
        bool started = false;
        bool recording = false;
        bool streaming = false;
        bool doRecord = false;
        bool doStream = false;
        bool needsFlushing = false;
        bool streamPaused = false;

        thread* recordingThread = 0;
        thread* streamingThread = 0;
#ifdef _WIN32
	    ALCdevice* device = 0;
#else
	    ALCdevice_struct* device = 0;
#endif
	    VRSoundPtr recordingSound;

	    list<VRSoundBufferPtr> frameBuffer;
	    bool deviceOk = false;
	    VRMutex* streamMutex = 0;
	    int queueSize = 10;
	    int maxBufferSize = 20;
	    int streamBuffer = 5;
	    int queuedFrames = 0; // frames recorded but not streamed yet
	    int queuedStream = 0; // frames streamed at the beginning

	    void setup();
	    void start();
	    void stop();

	    void startRecordingThread();
	    void startStreamingThread();

	public:
		VRMicrophone();
		~VRMicrophone();

		static VRMicrophonePtr create();
		VRMicrophonePtr ptr();

		void startRecording();
		VRSoundPtr stopRecording();

		void startStreaming(string address, int port);
		void startStreamingOver(VRNetworkClientPtr client);
		void pauseStreaming(bool p);
		void stopStreaming();
};

OSG_END_NAMESPACE;

#endif //VRMICROPHONE_H_INCLUDED
