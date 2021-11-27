#include "VRPySound.h"
#include "VRPyBaseT.h"

#include "core/scene/sound/VRSoundManager.h"

using namespace OSG;

simpleVRPyType( SoundManager, 0 );
simpleVRPyType( Sound, New_ptr );
simpleVRPyType( Microphone, New_ptr );

PyMethodDef VRPySound::methods[] = {
    {"play", PyWrap(Sound, play, "Play sound", void) },
    {"stop", PyWrap(Sound, stop, "Stop sound", void) },
    {"pause", PyWrap(Sound, pause, "Pause sound", void) },
    {"resume", PyWrap(Sound, resume, "Resume paused sound", void) },
    {"setPath", PyWrap(Sound, setPath, "Stop sound", void, string ) },
    {"setLoop", PyWrap(Sound, setLoop, "Stop sound", void, bool ) },
    {"setPitch", PyWrap(Sound, setPitch, "Stop sound", void, float ) },
    {"setVolume", PyWrap(Sound, setVolume, "Set sound volume, try setting master volume on soundmanager", void, float ) },
    {"setUser", PyWrap(Sound, setUser, "Stop sound", void, Vec3d, Vec3d ) },
    {"isRunning", PyWrap(Sound, isRunning, "Check if sound is running", bool) },
    {"synthesize", PyWrap(Sound, synthesize, "synthesize( Ac, wc, pc, Am, wm, pm, T)\t\n A,w,p are the amplitude, frequency and phase, c and m are the carrier sinusoid and modulator sinusoid, T is the packet duration in seconds", void, float, float, float, float, float, float, float) },
    {"synthBuffer", PyWrap(Sound, synthBuffer, "synthBuffer( [[f,A]], [[f,A]], T )\t\n [f,A] frequency/amplitude pairs, interpolate the two spectra, T is the packet duration in seconds", vector<short>, vector<Vec2d>, vector<Vec2d>, float) },
    {"synthBufferOnChannels", PyWrap(Sound, synthBufferOnChannels, "synthBufferOnChannels( [[[f,A]]], [[[f,A]]], T)\n\t [[f,A]] list of channels with each containing a list of frequency/amplitude pairs in channel order, interpolate the two spectra\n\tT is the packet duration in seconds\n\t", void, vector<vector<Vec2d>>, vector<vector<Vec2d>>, float) },
    {"synthSpectrum", PyWrap(Sound, synthSpectrum, "synthSpectrum( [A], int S, float T, float F, bool retBuffer )\t\n A amplitude, S sample rate, T packet duration in seconds, F fade in/out duration in s , specify if you want to return the generated buffer, maxQueued", vector<short>, vector<double>, uint, float, float, bool, int) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPySoundManager::methods[] = {
    {"setupSound", PyWrapOpt(SoundManager, setupSound, "Play sound, lopping and playing are optional", "0|0", VRSoundPtr, string, bool, bool) },
    {"stopAllSounds", PyWrap(SoundManager, stopAllSounds, "Stops all currently playing sounds.", void) },
    {"setVolume", PyWrap(SoundManager, setVolume, "Set sound volume from 0 to 1", void, float) },
    {"queueSounds", PyWrap(SoundManager, queueSounds, "Queue a list of sounds", void, vector<VRSoundPtr>) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyMicrophone::methods[] = {
    {"startRecording", PyWrap(Microphone, startRecording, "Starts to record from microphone", void) },
    {"stopRecording", PyWrap(Microphone, stopRecording, "Stops the recording, returns sound", VRSoundPtr) },
    {NULL}  /* Sentinel */
};
