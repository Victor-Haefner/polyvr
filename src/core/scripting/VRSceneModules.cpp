#include "VRSceneModules.h"
#include "VRSceneGlobals.h"
#include "VRScriptManagerT.h"

#include "VRPyMath.h"
#include "VRPyNamed.h"
#include "VRPyObject.h"
#include "VRPyGeometry.h"
#include "VRPyAnimation.h"
#include "VRPySocket.h"
#include "VRPySprite.h"
#ifndef WITHOUT_AV
#include "VRPySound.h"
#include "VRPyRecorder.h"
#endif
#include "VRPyDevice.h"
#include "VRPyIntersection.h"
#include "VRPyPose.h"
#include "VRPyPath.h"
#include "VRPyStateMachine.h"
#include "VRPyGraph.h"
#include "VRPyPolygon.h"
#include "VRPyBoundingbox.h"
#ifndef WITHOUT_GLU_TESS
#include "VRPyTriangulator.h"
#endif
#include "VRPyStroke.h"
#include "VRPyColorChooser.h"
#include "VRPyTextureRenderer.h"
#include "VRPyTextureMosaic.h"
#include "VRPyConstraint.h"
#include "VRPyMouse.h"
#include "VRPyMobile.h"
#include "VRPyBaseT.h"
#include "VRPyMaterial.h"
#include "VRPyTextureGenerator.h"
#include "VRPyLight.h"
#include "VRPyLightBeacon.h"
#ifndef WITHOUT_TCP
#include "VRPySyncNode.h"
#endif
#include "VRPyCamera.h"
#include "VRPyLod.h"
#include "VRPyKinematics.h"
#include "VRPyPathtool.h"
#include "VRPyConstructionKit.h"
#include "VRPySnappingEngine.h"
#include "VRPyAnnotationEngine.h"
#include "VRPyAnalyticGeometry.h"
#ifndef WITHOUT_PANGO_CAIRO
#include "VRPyPDF.h"
#endif
#include "VRPySelector.h"
#include "VRPySelection.h"
#include "VRPyPatchSelection.h"
#include "VRPyPolygonSelection.h"
#ifndef WITHOUT_BULLET
#include "VRPySpatialCollisionManager.h"
#endif
#include "VRPyMenu.h"
#include "VRPyClipPlane.h"
#include "VRPyListMath.h"
#include "VRPySetup.h"
#include "VRPyRendering.h"
#include "VRPyNavigator.h"
#include "VRPyNavPreset.h"
#include "VRPyWaypoint.h"
#include "VRPyMeasure.h"
#include "VRPyJointTool.h"
#include "VRPyImage.h"
#include "VRPyNetworking.h"
#include "VRPyProjectManager.h"
#include "VRPyGeoPrimitive.h"
#include "VRPyProgress.h"
#include "VRPyUndoManager.h"
#include "VRPyObjectManager.h"
#include "VRPySky.h"
#include "VRPyScenegraphInterface.h"
#include "VRPyOPCUA.h"
#include "VRPyCodeCompletion.h"
#include "VRPyPointCloud.h"

#include "addons/Character/VRPyCharacter.h"
#include "addons/Algorithms/VRPyGraphLayout.h"
#include "addons/Algorithms/VRPyPathFinding.h"
#include "addons/CaveKeeper/VRPyCaveKeeper.h"
#include "addons/Engineering/Factory/VRPyFactory.h"
#include "addons/Engineering/Factory/VRPyLogistics.h"
#include "addons/Engineering/Factory/VRPyProduction.h"
#include "addons/Engineering/Factory/VRPyAMLLoader.h"
#include "addons/Engineering/Mechanics/VRPyMechanism.h"
#include "addons/Engineering/Machining/VRPyMachining.h"
#include "addons/Engineering/VRPyNumberingEngine.h"
#include "addons/Semantics/Segmentation/VRPySegmentation.h"
#include "addons/Semantics/Segmentation/VRPyAdjacencyGraph.h"
#include "addons/Semantics/Processes/VRPyProcess.h"
#include "addons/Engineering/Chemistry/VRPyMolecule.h"
#include "addons/Engineering/Milling/VRPyMillingMachine.h"
#include "addons/Engineering/Milling/VRPyMillingWorkPiece.h"
#include "addons/Engineering/Milling/VRPyMillingCuttingToolProfile.h"
#include "addons/Engineering/VRPyRobotArm.h"
#include "addons/WorldGenerator/VRPyWorldGenerator.h"
#include "addons/WorldGenerator/nature/VRPyNature.h"
#include "addons/WorldGenerator/terrain/VRPyTerrain.h"
#include "addons/WorldGenerator/weather/VRPyWeather.h"
#ifndef WITHOUT_CGAL
#include "addons/Engineering/CSG/VRPyCSG.h"
#endif
#include "addons/SimViDekont/VRPySimViDekont.h"
#include "addons/Semantics/Reasoning/VRPyOntology.h"
#include "addons/LeapMotion/VRPyHandGeo.h"
#include "addons/LeapMotion/VRPyLeap.h"

#ifndef WITHOUT_MTOUCH
#include "VRPyMultiTouch.h"
#endif

#ifndef WITHOUT_BULLET
#ifndef WITHOUT_VIRTUOSE
#include "VRPyHaptic.h"
#endif
#include "addons/Bullet/Particles/VRPyParticles.h"
#include "addons/Bullet/Fluids/VRPyFluids.h"
#include "addons/Bullet/CarDynamics/VRPyCarDynamics.h"
#include "addons/WorldGenerator/traffic/VRPyTrafficSimulation.h"
#endif

#ifndef WITHOUT_CEF
#include "addons/CEF/VRPyCEF.h"
#ifndef _WIN32
#include "addons/CEF/VRPyWebCam.h"
#endif
#endif

#ifndef WITHOUT_CRYPTOPP
#include "VRPyEncryption.h"
#endif

using namespace OSG;

void VRSceneModules::setup(VRScriptManager* sm, PyObject* pModVR) {
    sm->registerModule<VRPyName>("Named", pModVR, VRPyStorage::typeRef);
    sm->registerModule<VRPyObject>("Object", pModVR, VRPyName::typeRef);
    sm->registerModule<VRPyTransform>("Transform", pModVR, VRPyObject::typeRef);
    sm->registerModule<VRPyGeometry>("Geometry", pModVR, VRPyTransform::typeRef);
#ifndef WITHOUT_BULLET
    sm->registerModule<VRPySpatialCollisionManager>("SpatialCollisionManager", pModVR, VRPyGeometry::typeRef);
#endif
    sm->registerModule<VRPyMaterial>("Material", pModVR, VRPyObject::typeRef);
    sm->registerModule<VRPyTextureGenerator>("TextureGenerator", pModVR);
    sm->registerModule<VRPyTexture>("Image", pModVR);
    sm->registerModule<VRPyLight>("Light", pModVR, VRPyObject::typeRef);
    sm->registerModule<VRPyLightBeacon>("LightBeacon", pModVR, VRPyTransform::typeRef);
#ifndef WITHOUT_TCP
    sm->registerModule<VRPySyncNode>("SyncNode", pModVR, VRPyTransform::typeRef);
    //sm->registerModule<VRPySyncRemote>("SyncRemote", pModVR);
#endif
    sm->registerModule<VRPyCamera>("Camera", pModVR, VRPyTransform::typeRef);
#ifndef WITHOUT_BULLET
    sm->registerModule<VRPyKinematics>("Kinematics", pModVR, VRPyTransform::typeRef);
#endif
    sm->registerModule<VRPyFABRIK>("FABRIK", pModVR);
    sm->registerModule<VRPyLod>("Lod", pModVR, VRPyObject::typeRef);
    sm->registerModule<VRPyLodLeaf>("LodLeaf", pModVR, VRPyTransform::typeRef);
    sm->registerModule<VRPyLodTree>("LodTree", pModVR, VRPyObject::typeRef);
    sm->registerModule<VRPySprite>("Sprite", pModVR, VRPyGeometry::typeRef);
    sm->registerModule<VRPyPointCloud>("PointCloud", pModVR, VRPyTransform::typeRef);
#ifndef WITHOUT_AV
    sm->registerModule<VRPySound>("Sound", pModVR);
    sm->registerModule<VRPyVideo>("Video", pModVR);
    sm->registerModule<VRPySoundManager>("SoundManager", pModVR);
    sm->registerModule<VRPyRecorder>("Recorder", pModVR);
#endif
    sm->registerModule<VRPySocket>("Socket", pModVR);
    sm->registerModule<VRPyStroke>("Stroke", pModVR, VRPyGeometry::typeRef);
    sm->registerModule<VRPyConstraint>("Constraint", pModVR);
    sm->registerModule<VRPyDevice>("Device", pModVR, VRPyName::typeRef);
    sm->registerModule<VRPyIntersection>("Intersection", pModVR);
    sm->registerModule<VRPyServer>("Mobile", pModVR, VRPyDevice::typeRef);
    sm->registerModule<VRPyMouse>("Mouse", pModVR, VRPyDevice::typeRef);
    sm->registerModule<VRPyServer>("Server", pModVR, VRPyDevice::typeRef);
    sm->registerModule<VRPyAnimation>("Animation", pModVR);
    sm->registerModule<VRPyPose>("Pose", pModVR);
    sm->registerModule<VRPyPath>("Path", pModVR);
    sm->registerModule<VRPyGraph>("Graph", pModVR);
    sm->registerModule<VRPyDatarow>("Datarow", pModVR);
#ifndef WITHOUT_PANGO_CAIRO
    sm->registerModule<VRPyPDF>("PDF", pModVR);
#endif
    sm->registerModule<VRPyStateMachine>("StateMachine", pModVR);
#ifndef WITHOUT_HDLC
    sm->registerModule<VRPyHDLC>("HDLC", pModVR);
#endif
    sm->registerModule<VRPyRestResponse>("RestResponse", pModVR);
    sm->registerModule<VRPyRestClient>("RestClient", pModVR);
    sm->registerModule<VRPyRestServer>("RestServer", pModVR);
    sm->registerModule<VRPyState>("State", pModVR);
    sm->registerModule<VRPyGraphLayout>("GraphLayout", pModVR);
    sm->registerModule<VRPyPathFinding>("PathFinding", pModVR);
    sm->registerModule<VRPyBoundingbox>("Boundingbox", pModVR);
    sm->registerModule<VRPyPolygon>("Polygon", pModVR);
    sm->registerModule<VRPyFrustum>("Frustum", pModVR);
#ifndef WITHOUT_GLU_TESS
    sm->registerModule<VRPyTriangulator>("Triangulator", pModVR);
#endif
    sm->registerModule<VRPyProjectManager>("ProjectManager", pModVR, VRPyObject::typeRef);
    sm->registerModule<VRPyHandle>("Handle", pModVR, VRPyGeometry::typeRef);
    sm->registerModule<VRPyGeoPrimitive>("GeoPrimitive", pModVR, VRPyTransform::typeRef);
    sm->registerModule<VRPyStorage>("Storage", pModVR);
    sm->registerModule<VRPySnappingEngine>("SnappingEngine", pModVR);
    sm->registerModule<VRPyAnnotationEngine>("AnnotationEngine", pModVR, VRPyGeometry::typeRef);
    sm->registerModule<VRPyAnalyticGeometry>("AnalyticGeometry", pModVR, VRPyObject::typeRef);
    sm->registerModule<VRPyConstructionKit>("ConstructionKit", pModVR);
    sm->registerModule<VRPyPathtool>("Pathtool", pModVR, VRPyTransform::typeRef);
    sm->registerModule<VRPySelector>("Selector", pModVR);
    sm->registerModule<VRPySelection>("Selection", pModVR);
    sm->registerModule<VRPyPatchSelection>("PatchSelection", pModVR, VRPySelection::typeRef);
    sm->registerModule<VRPyPolygonSelection>("PolygonSelection", pModVR, VRPySelection::typeRef);
    sm->registerModule<VRPyNavigator>("Navigator", pModVR);
    sm->registerModule<VRPyNavPreset>("NavPreset", pModVR);
    sm->registerModule<VRPyRendering>("Rendering", pModVR);
    sm->registerModule<VRPyRenderStudio>("RenderStudio", pModVR);
    sm->registerModule<VRPySky>("Sky", pModVR, VRPyGeometry::typeRef);
    sm->registerModule<VRPyScenegraphInterface>("ScenegraphInterface", pModVR, VRPyObject::typeRef);
    sm->registerModule<VRPyProgress>("Progress", pModVR);
    sm->registerModule<VRPyUndoManager>("UndoManager", pModVR);
    sm->registerModule<VRPyObjectManager>("ObjectManager", pModVR, VRPyObject::typeRef);
    sm->registerModule<VRPyMenu>("Menu", pModVR, VRPyGeometry::typeRef);
    sm->registerModule<VRPyClipPlane>("ClipPlane", pModVR, VRPyGeometry::typeRef);
    sm->registerModule<VRPyWaypoint>("Waypoint", pModVR, VRPyGeometry::typeRef);
    sm->registerModule<VRPyMeasure>("Measure", pModVR, VRPyAnalyticGeometry::typeRef);
    sm->registerModule<VRPyJointTool>("JointTool", pModVR, VRPyGeometry::typeRef);
	sm->registerModule<VRPyColorChooser>("ColorChooser", pModVR);
	sm->registerModule<VRPyTextureRenderer>("TextureRenderer", pModVR, VRPyObject::typeRef);
	sm->registerModule<VRPyTextureMosaic>("TextureMosaic", pModVR, VRPyTexture::typeRef);
    sm->registerModule<VRPyCaveKeeper>("CaveKeeper", pModVR);
    sm->registerModule<VRPySegmentation>("Segmentation", pModVR);
    sm->registerModule<VRPyAdjacencyGraph>("AdjacencyGraph", pModVR);
    sm->registerModule<VRPyMechanism>("Mechanism", pModVR, VRPyObject::typeRef);
#ifndef WITHOUT_EIGEN
    sm->registerModule<VRPyGearSegmentation>("GearSegmentation", pModVR);
    sm->registerModule<VRPyAxleSegmentation>("AxleSegmentation", pModVR);
#endif
    sm->registerModule<VRPyMachiningSimulation>("MachiningSimulation", pModVR);
    sm->registerModule<VRPyMachiningCode>("MachiningCode", pModVR);
    sm->registerModule<VRPyMachiningKinematics>("MachiningKinematics", pModVR);
    sm->registerModule<VRPyCartesianKinematics>("CartesianKinematics", pModVR, VRPyMachiningKinematics::typeRef);
    sm->registerModule<VRPyNumberingEngine>("NumberingEngine", pModVR, VRPyGeometry::typeRef);
    sm->registerModule<VRPySkeleton>("Skeleton", pModVR, VRPyObject::typeRef);
    sm->registerModule<VRPyCharacter>("Character", pModVR, VRPyObject::typeRef);
    sm->registerModule<VRPyTree>("Tree", pModVR, VRPyGeometry::typeRef);
    sm->registerModule<VRPyNature>("Nature", pModVR, VRPyObject::typeRef);
    sm->registerModule<VRPyTerrain>("Terrain", pModVR, VRPyGeometry::typeRef);
    sm->registerModule<VRPyRain>("Rain", pModVR, VRPyGeometry::typeRef);
    sm->registerModule<VRPyRainCarWindshield>("RainCarWindshield", pModVR, VRPyGeometry::typeRef);
    sm->registerModule<VRPyPlanet>("Planet", pModVR, VRPyTransform::typeRef);
    sm->registerModule<VRPyOrbit>("Orbit", pModVR);
    sm->registerModule<VRPyMillingMachine>("MillingMachine", pModVR);
    sm->registerModule<VRPyMillingWorkPiece>("MillingWorkPiece", pModVR, VRPyGeometry::typeRef);
    sm->registerModule<VRPyMillingCuttingToolProfile>("MillingCuttingToolProfile", pModVR);
    sm->registerModule<VRPyMolecule>("Molecule", pModVR, VRPyGeometry::typeRef);
    sm->registerModule<VRPyCrystal>("Crystal", pModVR, VRPyMolecule::typeRef);
    sm->registerModule<VRPyRobotArm>("RobotArm", pModVR);
    sm->registerModule<VRPyOntology>("Ontology", pModVR, VRPyName::typeRef);
    sm->registerModule<VRPyProcess>("Process", pModVR, VRPyName::typeRef);
    sm->registerModule<VRPyProcessNode>("ProcessNode", pModVR, VRPyName::typeRef);
    sm->registerModule<VRPyProcessDiagram>("ProcessDiagram", pModVR, VRPyGraph::typeRef);
    sm->registerModule<VRPyProcessLayout>("ProcessLayout", pModVR, VRPyTransform::typeRef);
    sm->registerModule<VRPyProcessEngine>("ProcessEngine", pModVR);
    sm->registerModule<VRPyOntologyRule>("OntologyRule", pModVR);
    sm->registerModule<VRPyProperty>("Property", pModVR, VRPyName::typeRef);
    sm->registerModule<VRPyConcept>("Concept", pModVR, VRPyName::typeRef);
    sm->registerModule<VRPyEntity>("Entity", pModVR, VRPyName::typeRef);
    sm->registerModule<VRPyReasoner>("Reasoner", pModVR);
    sm->registerModule<VRPyScript>("Script", pModVR);

    sm->registerModule<VRPyHandGeo>("HandGeo", pModVR, VRPyGeometry::typeRef);
    sm->registerModule<VRPyLeap>("Leap", pModVR, VRPyDevice::typeRef);
    sm->registerModule<VRPyLeapFrame>("LeapFrame", pModVR);

    sm->registerModule<VRPyXML>("XML", pModVR);
    sm->registerModule<VRPyXMLElement>("XMLElement", pModVR);
    sm->registerModule<VRPySpreadsheet>("Spreadsheet", pModVR);

#ifndef WITHOUT_CGAL
	sm->registerModule<VRPyCSG>("CSGGeometry", pModVR, VRPyGeometry::typeRef);
#endif
	sm->registerModule<VRPySimViDekont>("SimViDekont", pModVR);

    PyObject* pModMath = sm->newModule("Math", VRPyMath::methods, "VR math module");
    sm->registerModule<VRPyVec2f>("Vec2", pModMath, 0, "Math");
    sm->registerModule<VRPyVec3f>("Vec3", pModMath, 0, "Math");
    sm->registerModule<VRPyLine>("Line", pModMath, 0, "Math");
    sm->registerModule<VRPyExpression>("Expression", pModVR);
    sm->registerModule<VRPyMathExpression>("MathExpression", pModMath, VRPyExpression::typeRef, "Math");
    sm->registerModule<VRPyTSDF>("TSDF", pModVR, 0, "Math");
    sm->registerModule<VRPyOctree>("Octree", pModVR, 0, "Math");
    sm->registerModule<VRPyOctreeNode>("OctreeNode", pModVR, 0, "Math");
#ifndef WITHOUT_LAPACKE_BLAS
    sm->registerModule<VRPyPCA>("PCA", pModVR, 0, "Math");
#endif
    sm->registerModule<VRPyPatch>("Patch", pModVR, 0, "Math");

    PyObject* pModSetup = sm->newModule("Setup", VRSceneGlobals::methods, "VR setup module");
    sm->registerModule<VRPySetup>("Setup", pModSetup, 0, "Setup");
    sm->registerModule<VRPyView>("View", pModSetup, 0, "Setup");
    sm->registerModule<VRPyWindow>("Window", pModSetup, 0, "Setup");

    PyObject* pModWorldGenerator = sm->newModule("WorldGenerator", VRSceneGlobals::methods, "VR world generator module");
    sm->registerModule<VRPyWorldGenerator>("WorldGenerator", pModWorldGenerator, VRPyTransform::typeRef, "WorldGenerator");
    sm->registerModule<VRPyAsphalt>("Asphalt", pModWorldGenerator, VRPyMaterial::typeRef, "WorldGenerator");
    sm->registerModule<VRPyRoadBase>("RoadBase", pModWorldGenerator, VRPyObject::typeRef, "WorldGenerator");
    sm->registerModule<VRPyRoad>("Road", pModWorldGenerator, VRPyRoadBase::typeRef, "WorldGenerator");
    sm->registerModule<VRPyRoadIntersection>("RoadIntersection", pModWorldGenerator, VRPyRoadBase::typeRef, "WorldGenerator");
    sm->registerModule<VRPyRoadNetwork>("RoadNetwork", pModWorldGenerator, VRPyRoadBase::typeRef, "WorldGenerator");
    sm->registerModule<VRPyTrafficSigns>("TrafficSigns", pModWorldGenerator, VRPyRoadBase::typeRef, "WorldGenerator");
    sm->registerModule<VRPyDistrict>("District", pModWorldGenerator, 0, "WorldGenerator");
    sm->registerModule<VRPyMapManager>("MapManager", pModVR, 0, "WorldGenerator");
    sm->registerModule<VRPyOSMMap>("OSMMap", pModVR, 0, "WorldGenerator");
    sm->registerModule<VRPyOSMRelation>("OSMRelation", pModVR, VRPyOSMBase::typeRef, "WorldGenerator");
    sm->registerModule<VRPyOSMWay>("OSMWay", pModVR, VRPyOSMBase::typeRef, "WorldGenerator");
    sm->registerModule<VRPyOSMNode>("OSMNode", pModVR, VRPyOSMBase::typeRef, "WorldGenerator");
    sm->registerModule<VRPyOSMBase>("OSMBase", pModVR, 0, "WorldGenerator");

    PyObject* pModFactory = sm->newModule("Factory", VRSceneGlobals::methods, "VR factory module");
    sm->registerModule<VRPyFNode>("Node", pModFactory, 0, "Factory");
    sm->registerModule<VRPyFNetwork>("Network", pModFactory, 0, "Factory");
    sm->registerModule<VRPyFPath>("FPath", pModFactory, 0, "Factory");
    sm->registerModule<VRPyFTransporter>("Transporter", pModFactory, 0, "Factory");
    sm->registerModule<VRPyFObject>("Product", pModFactory, 0, "Factory");
    sm->registerModule<VRPyFContainer>("Container", pModFactory, VRPyFObject::typeRef, "Factory");
    sm->registerModule<VRPyFProduct>("Product", pModFactory, VRPyFObject::typeRef, "Factory");
    sm->registerModule<VRPyFLogistics>("Logistics", pModFactory, 0, "Factory");
    sm->registerModule<VRPyFactory>("Factory", pModFactory, 0, "Factory");
    sm->registerModule<VRPyProduction>("Production", pModFactory, 0, "Factory");
    sm->registerModule<VRPyAMLLoader>("AMLLoader", pModFactory, 0, "Factory");

#ifndef WITHOUT_BULLET
    sm->registerModule<VRPyCollision>("Collision", pModVR);
#ifndef WITHOUT_VIRTUOSE
    sm->registerModule<VRPyHaptic>("Haptic", pModVR, VRPyDevice::typeRef);
#endif
    sm->registerModule<VRPyParticles>("Particles", pModVR, VRPyGeometry::typeRef);
    sm->registerModule<VRPyFluids>("Fluids", pModVR, VRPyParticles::typeRef);
    sm->registerModule<VRPyMetaBalls>("MetaBalls", pModVR, VRPyObject::typeRef);
    sm->registerModule<VRPyCarDynamics>("CarDynamics", pModVR, VRPyObject::typeRef);
#ifndef WITHOUT_AV
    sm->registerModule<VRPyCarSound>("CarSound", pModVR);
#endif
    sm->registerModule<VRPyDriver>("Driver", pModVR);
    sm->registerModule<VRPyTrafficSimulation>("TrafficSimulation", pModVR, VRPyObject::typeRef);
#endif

#ifdef WITH_OPCUA
    sm->registerModule<VRPyOPCUA>("OPCUA", pModVR);
    sm->registerModule<VRPyOPCUANode>("OPCUANode", pModVR);
#endif

#ifndef WITHOUT_CRYPTOPP
    sm->registerModule<VRPyEncryption>("Encryption", pModVR);
#endif

#ifndef WITHOUT_MTOUCH
    sm->registerModule<VRPyMultiTouch>("MultiTouch", pModVR, VRPyDevice::typeRef);
#endif

#ifndef WITHOUT_CEF
    sm->registerModule<VRPyCEF>("CEF", pModVR);
#ifndef _WIN32
    sm->registerModule<VRPyWebCam>("Webcam", pModVR, VRPySprite::typeRef);
#endif
#endif
}




