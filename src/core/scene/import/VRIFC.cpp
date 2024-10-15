#ifndef NO_IFC

#include "VRIFC.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/geometry/sprite/VRSprite.h"
#include "core/utils/toString.h"
#include "core/scene/import/VRSTEPCascade.h"
#include "core/math/pose.h"
#include "addons/Semantics/Reasoning/VROntology.h"

#include <boost/algorithm/string/join.hpp>
#include <ifcparse/IfcFile.h>
#include <ifcparse/IfcHierarchyHelper.h>
#include <ifcgeom/IfcGeom.h>
#include <ifcgeom/IfcGeomFilter.h>
#include <ifcgeom/IfcGeomIterator.h>
#include <ifcgeom/IfcGeomElement.h>
#include <ifcgeom/IfcGeomMaterial.h>
#include <ifcgeom/IfcGeomIteratorSettings.h>
#include <ifcgeom/IfcRepresentationShapeItem.h>


#include <fstream>
#include <sstream>
#include <set>
#include <time.h>

using namespace OSG;
using namespace IfcSchema;

// TODO:
// where are those:
//   #77484= IFCRELSPACEBOUNDARY('3iG9WO1yvn7Q0liDeNczIp',#12,'2ndLevel','2a',#33774,#27421,#77483,.PHYSICAL.,.EXTERNAL.);

class IFCLoader {
    private:
        //Color3f defaultColor = Color3f(1,1,1);
        Color3f defaultColor = Color3f(1,0,1);
        IfcGeom::Kernel kernel;
        VRTransformPtr root;

        VROntologyPtr ontology;
        map<IfcUtil::IfcBaseEntity*, VRObjectPtr> ifcObjects;

        Vec3d toVec3d(IfcCartesianPoint* p) {
            if (p->Coordinates().size() != 3) return Vec3d();
            return Vec3d( p->Coordinates()[0], p->Coordinates()[1], p->Coordinates()[2] );
        }

        Vec3d toVec3d(IfcDirection* p) {
            if (p->DirectionRatios().size() != 3) return Vec3d();
            return Vec3d( p->DirectionRatios()[0], p->DirectionRatios()[1], p->DirectionRatios()[2] );
        }

        PosePtr toPose(IfcAxis2Placement3D* p) {
            auto r = Pose::create();
            if (p->hasAxis()) r->setDir( toVec3d( p->Axis() ) );
            if (p->hasRefDirection()) r->setUp( r->dir().cross( toVec3d( p->RefDirection() ) ) );
            r->setPos( toVec3d(p->Location()) );
            return r;
        }

        void applyTransformation(const IfcGeom::TriangulationElement<float>* o, VRGeometryPtr geo) {
            IfcGeom::Transformation<float> trans = o->transformation();
            const IfcGeom::Matrix<float>& m = trans.matrix();
            const vector<float> v = m.data();
            //Matrix4d osgM = Matrix4d(v[0], v[1], v[2], v[9], v[3], v[4], v[5], v[10], v[6], v[7], v[8], v[11], 0, 0, 0, 1);
            Matrix4d osgM = Matrix4d(v[0], v[3], v[6], v[9],
                                     v[1], v[4], v[7], v[10],
                                     v[2], v[5], v[8], v[11],
                                     0,    0,    0,    1);
            geo->setMatrix(osgM);
        }

        Color3f getColor(int faceID, vector<int>& mat_ids, vector<IfcGeom::Material>& materials) {
            if (faceID < 0 || faceID >= int(mat_ids.size())) return defaultColor;
            int matID = mat_ids[faceID];
            if (matID < 0 || matID >= int(materials.size())) return defaultColor;
            IfcGeom::Material& mat = materials[matID];
            return Color3f( mat.diffuse()[0], mat.diffuse()[1], mat.diffuse()[2] );
        }

        string argVal(Argument* arg) {
            if (!arg->isNull()) {
                IfcUtil::ArgumentType type = arg->type();
                switch (type) {
                    case IfcUtil::ArgumentType::Argument_STRING: return string((string)*arg);
                    case IfcUtil::ArgumentType::Argument_ENUMERATION: return string((string)*arg);
                    case IfcUtil::ArgumentType::Argument_INT:
                        {int i = *arg;
                        return toString(i);}
                    case IfcUtil::ArgumentType::Argument_DOUBLE:
                        {double i = *arg;
                        return toString(i);}
                    case IfcUtil::ArgumentType::Argument_BOOL:
                        {bool i = *arg;
                        return toString(i);}
                    case IfcUtil::ArgumentType::Argument_ENTITY_INSTANCE:
                        {IfcUtil::IfcBaseClass* iobj = *arg;
                        int baseCount = iobj->getArgumentCount();
                        for (int bc = 0; bc < baseCount; bc++) {
                            return argVal(iobj->getArgument(bc));
                        }}
                    case IfcUtil::ArgumentType::Argument_AGGREGATE_OF_ENTITY_INSTANCE:
                        return "";
                    case IfcUtil::ArgumentType::Argument_NULL:
                        return "";
                    case IfcUtil::ArgumentType::Argument_DERIVED:
                        return "";
                    case IfcUtil::ArgumentType::Argument_BINARY:
                        return "";
                    case IfcUtil::ArgumentType::Argument_EMPTY_AGGREGATE:
                        return "";
                    case IfcUtil::ArgumentType::Argument_AGGREGATE_OF_INT:
                        return "";
                    case IfcUtil::ArgumentType::Argument_AGGREGATE_OF_DOUBLE:
                        return "";
                    case IfcUtil::ArgumentType::Argument_AGGREGATE_OF_STRING:
                        return "";
                    case IfcUtil::ArgumentType::Argument_AGGREGATE_OF_BINARY:
                        return "";
                    case IfcUtil::ArgumentType::Argument_AGGREGATE_OF_EMPTY_AGGREGATE:
                        return "";
                    case IfcUtil::ArgumentType::Argument_AGGREGATE_OF_AGGREGATE_OF_INT:
                        return "";
                    case IfcUtil::ArgumentType::Argument_AGGREGATE_OF_AGGREGATE_OF_DOUBLE:
                        return "";
                    case IfcUtil::ArgumentType::Argument_AGGREGATE_OF_AGGREGATE_OF_ENTITY_INSTANCE:
                        return "";
                    case IfcUtil::ArgumentType::Argument_UNKNOWN:
                        return "";
                }
            }
            return "";
        }

        vector<VREntityPtr> getEntities(IfcObject::list::ptr relObjs) {
            vector<VREntityPtr> res;
            for ( IfcObject* obj : *relObjs ) {
                if ( ifcObjects.count( obj ) ) {
                    auto o = ifcObjects[ obj ];
                    if (o) if (auto e = o->getEntity()) res.push_back(e);
                }
            }
            return res;
        }

        void addProperty(VREntityPtr e, IfcUtil::IfcBaseEntity* prop) {
            map<string, string> paramData;
            int c = prop->getArgumentCount();
            for (int i = 0; i < c; i++) {
                string name = prop->getArgumentName(i);
                Argument* arg = prop->getArgument(i);
                paramData[name] = argVal(arg);
            }

            if (paramData.count("Name") && paramData.count("NominalValue")) {
                string n = paramData["Name"];
                string v = paramData["NominalValue"];
                e->add("property", n+": "+v);
            }
        }

        void processDefines(IfcRelDefines* defines) {
            auto entities = getEntities(defines->RelatedObjects());

            IfcRelDefinesByProperties* by_prop = defines->as<IfcRelDefinesByProperties>();
            if (by_prop) {
                auto pdef = by_prop->RelatingPropertyDefinition();
                if (IfcPropertySet* pset = pdef->as<IfcPropertySet>()) {
                    auto props = pset->HasProperties();

                    for (IfcUtil::IfcBaseEntity* prop : *props) {
                        for (auto e : entities) addProperty(e, prop);
                    }
                } else if(IfcElementQuantity* pset = pdef->as<IfcElementQuantity>()) {
                    auto props = pset->Quantities();

                    for (IfcUtil::IfcBaseEntity* prop : *props) {
                        for (auto e : entities) addProperty(e, prop);
                    }
                }
            }

            IfcRelDefinesByType* by_type = defines->as<IfcRelDefinesByType>();
            if (by_type) { // TODO
            }
        }

        VREntityPtr getEntity(IfcUtil::IfcBaseEntity* r) {
            if (!r) return 0;
            if (!ifcObjects.count(r)) return 0;
            auto obj = ifcObjects[r];
            if (!obj) return 0;
            auto e = obj->getEntity();
            if (!e) return 0;
            return e;
        }

        VRTransformPtr convertIfcSurface(IfcSurfaceOrFaceSurface* s) {
            if (!s) return 0;
            TopoDS_Shape shape;
            bool r = kernel.convert_shape(s, shape);
            return convertSTEPShape(shape);
        }

        void processConnects(IfcRelConnects* connects) {
            //cout << "processConnects " << Type::ToString(connects->type()) << endl;

            auto e = setupEntity(0, connects, "Relation");

            IfcRelSpaceBoundary* sb = connects->as<IfcRelSpaceBoundary>();
            if (sb) {
                if (sb->hasRelatedBuildingElement()) {
                    auto eBElement = getEntity( sb->RelatedBuildingElement() );
                    if (eBElement) {
                        e->set("related", eBElement->getName());
                        eBElement->add("relation", e->getName());
                    }
                }

                auto eSpace = getEntity( (IfcUtil::IfcBaseEntity*) sb->RelatingSpace() );
                if (!eSpace) return;

                e->set("relating", eSpace->getName());
                eSpace->add("relation", e->getName());
                auto spaceGeo = eSpace->getSGObject();

                if (!sb->hasConnectionGeometry()) return;
                VRTransformPtr geo;

                IfcConnectionGeometry* cGeo = sb->ConnectionGeometry();
                IfcConnectionSurfaceGeometry* cSurf = cGeo->as<IfcConnectionSurfaceGeometry>();

                if (cSurf) {
                    geo = convertIfcSurface( cSurf->SurfaceOnRelatingElement() );
                    /*if (cSurf->hasSurfaceOnRelatedElement()) { // TODO
                        geo2 = convertIfcSurface( cSurf->SurfaceOnRelatedElement() );
                    }*/
                }

                if (geo) {
                    root->addChild(geo);
                    if (spaceGeo) spaceGeo->addChild(geo);
                    e->setSGObject(geo);
                    geo->setEntity(e);
                    geo->hide();
                }
            }
        }

        void processProperty(IfcRelationship* r) {
            IfcRelDefines*  rDefs = r->as<IfcRelDefines>();
            IfcRelConnects* rCons = r->as<IfcRelConnects>();

            if (rDefs) processDefines( rDefs );
            //if (r->is(IfcRelAssigns))       processAssigns( r->as<IfcRelAssigns>() );
            //if (r->is(IfcRelAssociates))    processAssociates( r->as<IfcRelAssociates>() );
            if (rCons) processConnects( rCons );
            //if (r->is(IfcRelDeclares))      processDeclares( r->as<IfcRelDeclares>() );
            //if (r->is(IfcRelDecomposes))    processDecomposes( r->as<IfcRelDecomposes>() );
        }

        VREntityPtr setupEntity(VRObjectPtr obj, IfcRoot* element, string type) {
            auto e = ontology->addEntity( getName(element), type);
            if (obj) e->setSGObject(obj);
            if (obj) obj->setEntity(e);
            e->set("type", Type::ToString(element->type()));
            e->set("gID", element->GlobalId());
            //cout << " " << Type::ToString(element->type()) << ", " << element->Name() << ", " << element->GlobalId() << endl;
            return e;
        }

        VRGeometryPtr convertGeo(const IfcGeom::TriangulationElement<float>* o) {
            if (!o) {
                //cout << "Warning! no valid object passed to IFC::convertGeo, " << o << endl;
                return 0;
            }

            const IfcGeom::Representation::Triangulation<float>& mesh = o->geometry();

            vector<float> verts = mesh.verts();
            vector<float> norms = mesh.normals();
            vector<int> faces = mesh.faces();
            vector<int> mat_ids = mesh.material_ids();
            vector<IfcGeom::Material> materials = mesh.materials();
            //vector<const IfcGeom::Element<float>*> parents = o->parents();

            if (faces.size() == 0) return 0;

            VRGeoData data;
            for (uint i=0; i<verts.size(); i+=3) data.pushPos( Pnt3d(verts[i], verts[i+1], verts[i+2]) );
            for (uint i=0; i<norms.size(); i+=3) data.pushNorm( Vec3d(norms[i], norms[i+1], norms[i+2]) );
            for (uint i=0; i<faces.size(); i+=3) data.pushTri( faces[i], faces[i+1], faces[i+2] );

            vector<Color3f> cols = vector<Color3f>(verts.size()/3, defaultColor); // TODO: replace by material system!
            int Nfaces = faces.size()/3;
            //cout << "Mat " << Nfaces << " " << mat.original_name() << " " << color << endl;
            for (int i=0; i<Nfaces; i++) {
                Color3f color = getColor(i, mat_ids, materials);
                cols[faces[3*i  ]] = color;
                cols[faces[3*i+1]] = color;
                cols[faces[3*i+2]] = color;
            }
            for (uint i=0; i<cols.size(); i++) data.pushColor( cols[i] );

            string name = o->name();// + "_" + o->unique_id();
            auto geo = data.asGeometry(name);
            applyTransformation(o,geo);
            return geo;
        }

        IfcGeom::BRepElement<float>* createBRep(IfcRepresentation* representation, IfcProduct* product) {
            IfcGeom::IteratorSettings settings;
            settings.set(IfcGeom::IteratorSettings::NO_NORMALS,                   false);
            settings.set(IfcGeom::IteratorSettings::WELD_VERTICES,                false);
            settings.set(IfcGeom::IteratorSettings::APPLY_DEFAULT_MATERIALS,      true);
            settings.set(IfcGeom::IteratorSettings::USE_WORLD_COORDS,             false);
            settings.set(IfcGeom::IteratorSettings::SEW_SHELLS,                   false);
            settings.set(IfcGeom::IteratorSettings::CONVERT_BACK_UNITS,           false);
            settings.set(IfcGeom::IteratorSettings::DISABLE_OPENING_SUBTRACTIONS, false);
            settings.set(IfcGeom::IteratorSettings::INCLUDE_CURVES,               false);
            settings.set(IfcGeom::IteratorSettings::EXCLUDE_SOLIDS_AND_SURFACES,  false);
            settings.set(IfcGeom::IteratorSettings::APPLY_LAYERSETS,              false);
            settings.set(IfcGeom::IteratorSettings::GENERATE_UVS,                 false);
            settings.set(IfcGeom::IteratorSettings::SEARCH_FLOOR,                 true);
            settings.set(IfcGeom::IteratorSettings::SITE_LOCAL_PLACEMENT,         false);
            settings.set(IfcGeom::IteratorSettings::BUILDING_LOCAL_PLACEMENT,     false);

            IfcGeom::BRepElement<float>* element = kernel.create_brep_for_representation_and_product<float>(settings, representation, product);
            if (!element) {
                //cout << "Warning in IFC::createBRep, create brep failed! " << Type::ToString(representation->type()) << " " << Type::ToString(product->type()) << endl;
                /*Logger::SetOutput( &cout, &cout );
                kernel.create_brep_for_representation_and_product<float>(settings, representation, product); // try again with logging
                Logger::SetOutput( 0, 0 );*/
            }
            return element;
		}

        IfcGeom::TriangulationElement<float>* createMesh(IfcGeom::BRepElement<float>* brep) {
			IfcGeom::TriangulationElement<float>* triangulation = 0;
			if (brep) triangulation = new IfcGeom::TriangulationElement<float>(*brep);
			//else  cout << "Warning! no valid object passed to IFC::createMesh, " << brep << endl;
			return triangulation;
		}

		string getName(IfcRoot* e) {
            string name = "UNNAMED";
            if (e->hasName()) name = e->Name();
            return name;
		}

		void analyseElement(IfcParse::IfcFile& file, int ID) {
            auto e = file.entityById(ID);
            if (!e) return;
            cout << "analyse element " << ID << ":" << endl;
            cout << " type: " << Type::ToString(e->type()) << endl;
            //if (e->hasName()) cout << " name: " << e->Name() << endl;
		}

    public:
        IFCLoader() {
            static VROntologyPtr o = 0;
            if (!o) {
                o = VROntology::create("BIM");
                VROntology::library["BIM"] = o;
                auto element = o->addConcept("Element");
                auto property = o->addConcept("Property", "Element");
                auto relation = o->addConcept("Relation", "Element");

                element->addProperty("type", "string");
                element->addProperty("gID", "string");
                element->addProperty("property", property->getName());
                element->addProperty("relation", relation->getName());

                property->addProperty("value", "string");

                relation->addProperty("relating", element->getName());
                relation->addProperty("related", element->getName());
                relation->addProperty("geometry", element->getName());
            }

            ontology = o;
        }

        void load(string path, VRTransformPtr res) {
            root = res;

            Logger::SetOutput( 0, &cout ); // Redirect the output (both progress and log) to stdout
            //Logger::SetOutput( 0, 0 ); // Redirect the output (both progress and log) to stdout
            Logger::Verbosity( Logger::LOG_NOTICE );
            //Logger::Verbosity( Logger::LOG_WARNING );
            //Logger::Verbosity( Logger::LOG_ERROR );

            IfcParse::IfcFile file;
            if ( !file.Init(path) ) { cout << "Unable to parse .ifc file: " << path << endl; return; }


            // create objects
            auto products = file.entitiesByType<IfcProduct>();
            cout << "Found " << products->size() << " products in " << path << endl;

            for ( IfcProduct* product : *products ) {
                //cout << " product " << product->id() << endl;

                if (!product->hasName()) continue;

                VRObjectPtr obj;

                if (product->hasRepresentation()) {
                    IfcProductRepresentation* prod_rep = product->Representation();
                    if (prod_rep) {
                        auto reps = prod_rep->Representations();
                        for (auto rep : *reps) {
                            if (!rep) continue;
                            auto ifcBRep = createBRep(rep, product);
                            auto ifcMesh = createMesh(ifcBRep);
                            auto geo = convertGeo(ifcMesh);
                            if (geo) obj = geo;
                        }
                    }
                }

                if (!obj) obj = VRTransform::create( getName(product) );
                setupEntity(obj, product, "Element");
                ifcObjects[product] = obj;
            }

            // create scenegraph
            for (auto o : ifcObjects) {
                IfcProduct* e = o.first->as<IfcProduct>();
                IfcSchema::IfcObjectDefinition* parent_object = kernel.get_decomposing_entity(e);
                if (parent_object) {
                    if (ifcObjects.count(parent_object)) {
                        ifcObjects[parent_object]->addChild(o.second);
                        continue;
                    }
                }
                res->addChild(o.second);
            }

            // fix transformations as they are in world coordinates
            for (auto o : res->getChildren(true)) { // iterate depth last to get correct results
                auto t = dynamic_pointer_cast<VRTransform>(o);
                if (!t) continue;
                t->setMatrixTo(t->getMatrix(), res);
            }

            // hide openings
            for (auto o : ifcObjects) {
                if (o.first->is(Type::IfcOpeningElement)) {
                    auto g = dynamic_pointer_cast<VRGeometry>(o.second);
                    if (g) g->setMeshVisibility(false);
                }
            }

            // check for relationships
            auto relationships = file.entitiesByType<IfcRelationship>();
            cout << "Found " << relationships->size() << " relationships" << endl;
            for (auto& r : *relationships) processProperty(r);

            //analyseElement(file, 83008);
        }
};

void OSG::loadIFC(string path, VRTransformPtr res) {
   IFCLoader ifc;
   ifc.load(path, res);

}

#endif
