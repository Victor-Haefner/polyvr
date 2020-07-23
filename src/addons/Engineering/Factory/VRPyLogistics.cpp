#include "VRPyLogistics.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/scripting/VRPyTypeCaster.h"
#include "core/scripting/VRPyTransform.h"
#include "core/scripting/VRPyObject.h"
#include "core/scripting/VRPyStroke.h"

using namespace OSG;

simplePyType( FNode, 0);
simplePyType( FNetwork, 0);
simplePyType( FPath, 0);
simplePyType( FTransporter, 0);
simplePyType( FContainer, 0);
simplePyType( FObject, 0);
simplePyType( FProduct, 0);
simplePyType( FLogistics, New_ptr);

PyMethodDef VRPyFNode::methods[] = {
    {"set", PyWrap2(FNode, set, "Set the node content", void, FObjectPtr ) },
    {"setTransform", PyWrap2(FNode, setTransform, "Set the node transformation", void, VRTransformPtr ) },
    {"getTransform", PyWrap2(FNode, getTransform, "Return the node transformation", VRTransformPtr ) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyFNetwork::methods[] = {
    {"addNode", PyWrap2(FNetwork, addNode, "Add new node to network", int, PosePtr) },
    {"connect", PyWrap2(FNetwork, connect, "Connect nodes", void, int, int) },
    {"stroke", PyWrap2(FNetwork, stroke, "Stroke the network", VRStrokePtr, Color3f, float) },
    {"getGraph", PyWrap2(FNetwork, getGraph, "Get the network graph", GraphPtr) },
    {"getNode", PyWrap2(FNetwork, getNode, "Get a node by ID", FNodePtr, int) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyFPath::methods[] = {
    {"first", PyWrap2(FPath, first, "Get first node", int) },
    {"last", PyWrap2(FPath, last, "Get last node", int) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyFTransporter::methods[] = {
    {"setPath", PyWrap2(FTransporter, setPath, "Set transporter path", void, FPathPtr) },
    {"setSpeed", PyWrap2(FTransporter, setSpeed, "Set transporter speed", void, float) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyFContainer::methods[] = {
    {"setCapacity", PyWrap2(FContainer, setCapacity, "Set container capacity", void, int) },
    {"getCapacity", PyWrap2(FContainer, getCapacity, "Set container capacity", int) },
    {"isEmpty", PyWrap2(FContainer, isEmpty, "Set container capacity", bool) },
    {"isFull", PyWrap2(FContainer, isFull, "Set container capacity", bool) },
    {"clear", PyWrap2(FContainer, clear, "Set container capacity", void) },
    {"getCount", PyWrap2(FContainer, getCount, "Get number of products in the container", int) },
    {"add", PyWrap2(FContainer, add, "Add a product to the container - add(product)", void, FProductPtr) },
    {"pop", PyWrap2(FContainer, pop, "Get the product last put in the container and remove it from the container", FProductPtr) },
    {"peek", PyWrap2(FContainer, peek, "Get the product last put in the container", FProductPtr) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyFObject::methods[] = {
    {"getTransformation", PyWrap2(FObject, getTransformation, "Get object transform", VRTransformPtr) },
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyFProduct::methods[] = {
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyFLogistics::methods[] = {
    {"addProduct", PyWrapOpt2(FLogistics, addProduct, "Add a new product from geometry", "0", FProductPtr, VRTransformPtr) },
    {"addNetwork", PyWrap2(FLogistics, addNetwork, "Add a new network", FNetworkPtr) },
    //{"addPath", PyWrap2(FLogistics, addPath, "Add a new path", void) },
    {"addTransporter", PyWrap2(FLogistics, addTransporter, "Add a new transporter", FTransporterPtr, string) },
    {"addContainer", PyWrap2(FLogistics, addContainer, "Add a new container", FContainerPtr, VRTransformPtr) },
    {"fillContainer", PyWrap2(FLogistics, fillContainer, "Fill container : fillContainer(container, N, obj)", void, FContainerPtr, int, VRTransformPtr) },
    {"update", PyWrap2(FLogistics, update, "Update logistics simulation", void) },
    {"clear", PyWrap2(FLogistics, clear, "Clear logistics simulation", void) },
    {"getContainers", PyWrap2(FLogistics, getContainers, "Destroy logistics simulation", vector<FContainerPtr>) },
    {"computeRoute", PyWrap2(FLogistics, computeRoute, "Compute route", FPathPtr, int, int) },
    {NULL}  /* Sentinel */
};





