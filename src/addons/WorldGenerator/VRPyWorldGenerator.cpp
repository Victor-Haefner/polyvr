#include "VRPyWorldGenerator.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/scripting/VRPyPath.h"
#include "addons/Semantics/Reasoning/VRPyOntology.h"

using namespace OSG;

simpleVRPyType(Asphalt, New_ptr);
simpleVRPyType(RoadNetwork, New_ptr);


PyMethodDef VRPyAsphalt::methods[] = {
    {"addMarking", (PyCFunction)VRPyAsphalt::addMarking, METH_VARARGS, "Add marking - addMarking( rID, path, width, dashN )" },
    {"addTrack", (PyCFunction)VRPyAsphalt::addTrack, METH_VARARGS, "Add track - addTrack( rID, path, width, dashN )" },
    {"clearTexture", (PyCFunction)VRPyAsphalt::clearTexture, METH_NOARGS, "Clear internal textures - clearTexture()" },
    {"updateTexture", (PyCFunction)VRPyAsphalt::updateTexture, METH_NOARGS, "Update internal textures - updateTexture()" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyAsphalt::addMarking(VRPyAsphalt* self, PyObject *args) {
    if (!self->valid()) return NULL;
    int rID = 0;
    int dashN = 0;
    float width = 0.1;
    VRPyPath* path = 0;
    if (!PyArg_ParseTuple(args, "iOf|i", &rID, &path, &width, &dashN)) return NULL;
    self->objPtr->addMarking( rID, path->objPtr, width, dashN );
    Py_RETURN_TRUE;
}

PyObject* VRPyAsphalt::addTrack(VRPyAsphalt* self, PyObject *args) {
    if (!self->valid()) return NULL;
    int rID = 0;
    int dashN = 0;
    float width = 0.1;
    VRPyPath* path = 0;
    if (!PyArg_ParseTuple(args, "iOf|i", &rID, &path, &width, &dashN)) return NULL;
    self->objPtr->addTrack( rID, path->objPtr, width, dashN );
    Py_RETURN_TRUE;
}

PyObject* VRPyAsphalt::clearTexture(VRPyAsphalt* self) {
    if (!self->valid()) return NULL;
    self->objPtr->clearTexture();
    Py_RETURN_TRUE;
}

PyObject* VRPyAsphalt::updateTexture(VRPyAsphalt* self) {
    if (!self->valid()) return NULL;
    self->objPtr->updateTexture();
    Py_RETURN_TRUE;
}


// ------------------------------------------------------

PyMethodDef VRPyRoadNetwork::methods[] = {
    {"setOntology", (PyCFunction)VRPyRoadNetwork::setOntology, METH_VARARGS, "Add a new node - setOntology( ontology )" },
    {"addNode", (PyCFunction)VRPyRoadNetwork::addNode, METH_VARARGS, "Add a new node - addNode( [x,y,z] )" },
    {"addLane", (PyCFunction)VRPyRoadNetwork::addLane, METH_VARARGS, "Add a new lane - addLane( int direction, road, float width )" },
    {"addRoad", (PyCFunction)VRPyRoadNetwork::addRoad, METH_VARARGS, "Add a new road - addRoad( str name, [paths], int rID, str type )" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyRoadNetwork::setOntology(VRPyRoadNetwork* self, PyObject *args) {
    if (!self->valid()) return NULL;
    VRPyOntology* ontology = 0;
    if (!PyArg_ParseTuple(args, "O", &ontology)) return NULL;
    self->objPtr->setOntology( ontology->objPtr );
    Py_RETURN_TRUE;
}

PyObject* VRPyRoadNetwork::addNode(VRPyRoadNetwork* self, PyObject *args) {
    if (!self->valid()) return NULL;
    auto n = self->objPtr->addNode( parseVec3f( args ) );
    return VRPyEntity::fromSharedPtr( n );
}

PyObject* VRPyRoadNetwork::addLane(VRPyRoadNetwork* self, PyObject *args) {
    if (!self->valid()) return NULL;
    int dir = 0;
    VRPyEntity* road = 0;
    float width = 0;
    if (!PyArg_ParseTuple(args, "iOf", &dir, &road, &width)) return NULL;
    auto l = self->objPtr->addLane( dir, road->objPtr, width );
    return VRPyEntity::fromSharedPtr( l );
}

PyObject* VRPyRoadNetwork::addRoad(VRPyRoadNetwork* self, PyObject *args) {
    if (!self->valid()) return NULL;
    const char* name;
    PyObject* paths = 0;
    int rID = 0;
    const char* type;
    if (!PyArg_ParseTuple(args, "sOis", &name, &paths, &rID, &type)) return NULL;
    vector<VREntityPtr> pathsV;
    for (int i=0; i<pySize(paths); i++) pathsV.push_back( ((VRPyEntity*)PyList_GetItem(paths, i))->objPtr );
    auto r = self->objPtr->addRoad( name?name:"", pathsV, rID, type?type:"" );
    return VRPyEntity::fromSharedPtr( r );
}



