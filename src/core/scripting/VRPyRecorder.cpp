#include "VRPyRecorder.h"
#include "VRPyBaseT.h"
#include "VRPyTransform.h"
#include "VRPyImage.h"
#include "VRPyMath.h"

using namespace OSG;

simpleVRPyType(Recorder, New_ptr);

PyMethodDef VRPyRecorder::methods[] = {
    {"capture", (PyCFunction)VRPyRecorder::capture, METH_NOARGS, "Capture the current view as an image - capture()" },
    {"compile", (PyCFunction)VRPyRecorder::compile, METH_VARARGS, "Compile the recorded frames to a video file on disk - compile(string path)" },
    {"setView", (PyCFunction)VRPyRecorder::setView, METH_VARARGS, "Set the view to record - setView(int ID)" },
    {"clear", (PyCFunction)VRPyRecorder::clear, METH_NOARGS, "Clear the captures frames - clear()" },
    {"getRecordingSize", (PyCFunction)VRPyRecorder::getRecordingSize, METH_NOARGS, "Get the number of captured frames - int getRecordingSize()" },
    {"getRecordingLength", (PyCFunction)VRPyRecorder::getRecordingLength, METH_NOARGS, "Get the length in seconds fromt he first to last of the captured frames - float getRecordingLength()" },
    {"setMaxFrames", (PyCFunction)VRPyRecorder::setMaxFrames, METH_VARARGS, "Set the maximum number of frames" },
    {"frameLimitReached", (PyCFunction)VRPyRecorder::frameLimitReached, METH_NOARGS, "Check if the frame limit has been reached" },
    {"get", (PyCFunction)VRPyRecorder::get, METH_VARARGS, "Get capture i - get(int i)" },
    {"setTransform", (PyCFunction)VRPyRecorder::setTransform, METH_VARARGS, "Apply the transform of the camera pose of frame i - setTransform(transform t, int i)" },
    {"getFrom", (PyCFunction)VRPyRecorder::getFrom, METH_VARARGS, "Get the position of the camera pose of frame i - getFrom(int i)" },
    {"getDir", (PyCFunction)VRPyRecorder::getDir, METH_VARARGS, "Get the direction of the camera pose of frame i - getDir(int i)" },
    {"getAt", (PyCFunction)VRPyRecorder::getAt, METH_VARARGS, "Get the at vector of the camera pose of frame i - getAt(int i)" },
    {"getUp", (PyCFunction)VRPyRecorder::getUp, METH_VARARGS, "Get the up vector of the camera pose of frame i - getUp(int i)" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyRecorder::setTransform(VRPyRecorder* self, PyObject* args) {
	VRPyTransform* t; int i;
    if (!PyArg_ParseTuple(args, "Oi", &t, &i)) return NULL;
    self->objPtr->setTransform(t->objPtr,i);
    Py_RETURN_TRUE;
}

PyObject* VRPyRecorder::get(VRPyRecorder* self, PyObject* args) {
    int i = parseInt(args);
    return VRPyTexture::fromSharedPtr( self->objPtr->get(i) );
}

PyObject* VRPyRecorder::getFrom(VRPyRecorder* self, PyObject* args) { return toPyObject(self->objPtr->getFrom( parseInt(args) ) ); }
PyObject* VRPyRecorder::getDir(VRPyRecorder* self, PyObject* args) { return toPyObject(self->objPtr->getDir( parseInt(args) ) ); }
PyObject* VRPyRecorder::getAt(VRPyRecorder* self, PyObject* args) { return toPyObject(self->objPtr->getAt( parseInt(args) ) ); }
PyObject* VRPyRecorder::getUp(VRPyRecorder* self, PyObject* args) { return toPyObject(self->objPtr->getUp( parseInt(args) ) ); }

PyObject* VRPyRecorder::setMaxFrames(VRPyRecorder* self, PyObject* args) {
    if (self->objPtr) self->objPtr->setMaxFrames( parseInt(args) );
    Py_RETURN_TRUE;
}

PyObject* VRPyRecorder::frameLimitReached(VRPyRecorder* self) {
    return PyBool_FromLong( self->objPtr->frameLimitReached() );
}

PyObject* VRPyRecorder::getRecordingSize(VRPyRecorder* self) {
    int size = 0;
    if (self->objPtr) size = self->objPtr->getRecordingSize();
    return PyInt_FromLong(size);
}

PyObject* VRPyRecorder::getRecordingLength(VRPyRecorder* self) {
    float length = 0;
    if (self->objPtr) length = self->objPtr->getRecordingLength();
    return PyFloat_FromDouble(length);
}

PyObject* VRPyRecorder::clear(VRPyRecorder* self) {
    if (self->objPtr) self->objPtr->clear();
    Py_RETURN_TRUE;
}

PyObject* VRPyRecorder::setView(VRPyRecorder* self, PyObject* args) {
    if (self->objPtr) self->objPtr->setView( parseInt(args) );
    Py_RETURN_TRUE;
}

PyObject* VRPyRecorder::capture(VRPyRecorder* self) {
    if (self->objPtr) self->objPtr->capture();
    int size = 0;
    if (self->objPtr) size = self->objPtr->getRecordingSize();
    return PyInt_FromLong(size-1);
}

PyObject* VRPyRecorder::compile(VRPyRecorder* self, PyObject* args) {
    if (self->objPtr) self->objPtr->compile( parseString(args) );
    Py_RETURN_TRUE;
}
