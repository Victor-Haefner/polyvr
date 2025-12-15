#ifndef VRSCENEGLOBALS_H_INCLUDED
#define VRSCENEGLOBALS_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "VRPyBase.h"

OSG_BEGIN_NAMESPACE;

class VRSceneGlobals: public VRPyBase {
    private:
    public:
        VRSceneGlobals();

        static PyMethodDef methods[];

		static PyObject* exit(PyObject* self, PyObject *args);
		static PyObject* find(PyObject* self, PyObject *args);
		static PyObject* findByID(PyObject* self, PyObject *args);
		static PyObject* analyzeFile(PyObject* self, PyObject *args);
		static PyObject* loadGeometry(PyObject* self, PyObject *args, PyObject *kwargs);
		static PyObject* getLoadGeometryProgress(PyObject* self, PyObject *args);
		static PyObject* createPrimitive(PyObject* self, PyObject *args, PyObject *kwargs);
		static PyObject* exportToFile(PyObject* self, PyObject *args);
		static PyObject* pyTriggerScript(PyObject* self, PyObject *args);
		static PyObject* stackCall(PyObject* self, PyObject *args);
		static PyObject* openFileDialog(PyObject* self, PyObject *args);
		static PyObject* updateGui(PyObject* self, PyObject *args);
		static PyObject* render(PyObject* self, PyObject *args);
		static PyObject* getRoot(PyObject* self, PyObject *args);
		static PyObject* printOSG(PyObject* self, PyObject *args);
		static PyObject* getNavigator(PyObject* self, PyObject *args);
		static PyObject* getSetup(PyObject* self, PyObject *args);
		static PyObject* getRendering(PyObject* self, PyObject *args);
		static PyObject* loadScene(PyObject* self, PyObject *args);
		static PyObject* startThread(PyObject* self, PyObject *args);
		static PyObject* joinThread(PyObject* self, PyObject *args);
		static PyObject* getSystemDirectory(PyObject* self, PyObject *args);
		static PyObject* setPhysicsActive(PyObject* self, PyObject *args);
		static PyObject* setPhysicsTimestep(PyObject* self, PyObject *args);
		static PyObject* runTest(PyObject* self, PyObject *args);
		static PyObject* getSceneMaterials(PyObject* self, PyObject *args);
		static PyObject* getBackground(PyObject* self, PyObject *args);
		static PyObject* getSky(PyObject* self, PyObject *args);
		static PyObject* getSoundManager(PyObject* self, PyObject *args);
		static PyObject* getFrame(PyObject* self, PyObject *args);
		static PyObject* getFPS(PyObject* self, PyObject *args);
		static PyObject* getScript(PyObject* self, PyObject *args);
		static PyObject* importScene(PyObject* self, PyObject *args);
		static PyObject* getActiveCamera(PyObject* self, PyObject *args);
		static PyObject* testDWGArcs(PyObject* self, PyObject *args);
		static PyObject* setWindowTitle(PyObject* self, PyObject* args);
		static PyObject* sendToBrowser(PyObject* self, PyObject* args);
		static PyObject* getPlatform(PyObject* self, PyObject *args);
};

OSG_END_NAMESPACE;

#endif // VRSCENEGLOBALS_H_INCLUDED
