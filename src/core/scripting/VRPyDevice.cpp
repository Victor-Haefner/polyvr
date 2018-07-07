#include "VRPyDevice.h"
#include "VRPyMobile.h"
#include "VRPyMouse.h"
#include "VRPyMultiTouch.h"
#include "VRPyHaptic.h"
#include "addons/LeapMotion/VRPyLeap.h"
#include "VRPyTransform.h"
#include "VRPyGeometry.h"
#include "VRPySprite.h"
#include "core/objects/VRTransform.h"
#include "VRPyBaseT.h"
#include "VRPyTypeCaster.h"

using namespace OSG;

simpleVRPyType(Signal, 0)
simpleVRPyType(Device, New_named_ptr)

PyMethodDef VRPySignal::methods[] = {
    {NULL}  /* Sentinel */
};

PyMethodDef VRPyDevice::methods[] = {
    {"getBeacon", PyWrapOpt( Device, getBeacon, "Get device beacon. - DeviceBeacon getBeacon(int beaconId = 0)", "0", VRTransformPtr, int ) },
    {"setBeacon", PyWrapOpt( Device, setBeacon, "Set device beacon.", "0", void, VRTransformPtr, int ) },
    {"getTarget", PyWrap( Device, getTarget, "Get device target.", VRTransformPtr ) },
    {"setTarget", PyWrap( Device, setTarget, "Set device target.", void, VRTransformPtr ) },
    {"getKey", PyWrap( Device, key, "Get activated device key.", int ) },
    {"getState", PyWrap( Device, getState, "Get device state.", int ) },
    {"getKeyState", PyWrap( Device, b_state, "Get device key state.", int, int ) },
    {"getSlider", PyWrap( Device, s_state, "Get device slider state.", float, int ) },
    {"getMessage", PyWrap( Device, getMessage, "Get device received message.", string ) },
    {"getType", PyWrap(Device, getType, "Get device type.", string ) },
    {"setDnD", PyWrap( Device, setDnD, "Set drag && drop.", void, bool ) },
    {"intersect", PyWrapOpt(Device, intersect2, "Attempts to intersect the device beacon with the scene - \n"
                                                                    "bool intersect(Object scene, bool force, DeviceBeacon beacon, Vec3 dir)\n\n"
                                                                    "  return: True, if intersection successful, otherwise False\n\n"
                                                                    "  scene:  [optional] default=VR.Scene()\n"
                                                                    "          Specifies object in scene graph which is checked incl. its children.\n\n"
                                                                    "  force:  [optional] default=False\n"
                                                                    "          Forces reevaluation of intersect, if False existing intersection from same frame can be used.\n\n"
                                                                    "  beacon: [optional] default=device.getBeacon()\n"
                                                                    "          Specifies which beacon of the device will be intersected, in case of multiple beacons (Multitouch).\n\n"
                                                                    "  dir:    [optional] default=[0,0,-1]\n"
                                                                    "          Currently not implemented! Creates a beacon from device position in given direction for intersect.", "0|0|0|0 0 -1", bool, VRObjectPtr, bool, VRTransformPtr, Vec3d ) },
    {"getIntersected", PyWrap(Device, getIntersected, "Get device intersected object.", VRObjectPtr ) },
    {"getIntersection", PyWrap(Device, getIntersectionPoint, "Get device intersection point", Pnt3d ) },
    {"getIntersectionNormal", PyWrap(Device, getIntersectionNormal, "Get normal at intersection point", Vec3d ) },
    {"getIntersectionUV", PyWrap(Device, getIntersectionUV, "Get uv at intersection point", Vec2d ) },
    {"getIntersectionTriangle", PyWrap(Device, getIntersectionTriangle, "Get triangle at intersection point", Vec3i ) },
    {"addIntersection", PyWrap( Device, addIntersection, "Add device intersection node.", void, VRObjectPtr ) },
    {"remIntersection", PyWrap( Device, remIntersection, "Remove device intersection node.", void, VRObjectPtr ) },
    {"getDragged", PyWrap( Device, getDragged, "Get dragged object.", VRTransformPtr ) },
    {"getDragGhost", PyWrap( Device, getDragGhost, "Get drag ghost.", VRTransformPtr ) },
    {"drag", PyWrap( Device, drag, "Start to drag an object", void, VRObjectPtr ) },
    {"drop", PyWrap( Device, drop, "Drop any object", void ) },
    {"setSpeed", PyWrap( Device, setSpeed, "Set the navigation speed of the device", void, Vec2d ) },
    {"getSpeed", PyWrap( Device, getSpeed, "Get the navigation speed of the device", Vec2d ) },
    {"addSignal", PyWrap( Device, newSignal, "Add a new signal, key, state", VRSignalPtr, int, int ) },
    {"trigger", PyWrap( Device, change_button, "Trigger signal, key, state", void, int, int ) },
    {NULL}  /* Sentinel */
};

PyObject* VRPyDevice::fromSharedPtr(VRDevicePtr dev) {
    string type = dev->getType();
    if (type == "mouse") return VRPyMouse::fromSharedPtr( static_pointer_cast<VRMouse>(dev) );
    else if (type == "multitouch") return VRPyMultiTouch::fromSharedPtr( static_pointer_cast<VRMultiTouch>(dev) );
    else if (type == "leap") return VRPyLeap::fromSharedPtr( static_pointer_cast<VRLeap>(dev) );
    else if (type == "server") return VRPyServer::fromSharedPtr( static_pointer_cast<VRServer>(dev) );
    else if (type == "haptic") return VRPyHaptic::fromSharedPtr( static_pointer_cast<VRHaptic>(dev) );
    else if (type == "keyboard") return VRPyBaseT<VRDevice>::fromSharedPtr( dev );
    else if (type == "flystick") return VRPyBaseT<VRDevice>::fromSharedPtr( dev );
    cout << "\nERROR in VRPyTypeCaster::cast device: " << type << " not handled!\n";
    return VRPyBaseT<VRDevice>::fromSharedPtr(dev);
}

