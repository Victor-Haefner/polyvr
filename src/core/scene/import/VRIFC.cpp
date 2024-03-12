#ifndef NO_IFC

#include "VRIFC.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/utils/toString.h"
#include "addons/Semantics/Reasoning/VROntology.h"

#include <boost/algorithm/string/join.hpp>
#include <ifcparse/IfcFile.h>
//if compiling fails because of "ifcparse/IfcFile.h: No such file or directory"
//in polyvr-folder: "sudo ./ install"
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

typedef double real_t;

using namespace OSG;
using namespace IfcSchema;

/// @todo make the filters non-global
IfcGeom::entity_filter entity_filter; // Entity filter is used always by default.
IfcGeom::layer_filter layer_filter;
const string NAME_ARG = "Name", GUID_ARG = "GlobalId", DESC_ARG = "Description", TAG_ARG = "Tag";
boost::array<string, 4> supported_args = { { NAME_ARG, GUID_ARG, DESC_ARG, TAG_ARG } };
IfcGeom::string_arg_filter guid_filter(IfcSchema::Type::IfcRoot, 0); // IfcRoot.GlobalId
// Note: skipping IfcRoot OwnerHistory, argument index 1
IfcGeom::string_arg_filter name_filter(IfcSchema::Type::IfcRoot, 2); // IfcRoot.Name
IfcGeom::string_arg_filter desc_filter(IfcSchema::Type::IfcRoot, 3); // IfcRoot.Description
IfcGeom::string_arg_filter tag_filter(IfcSchema::Type::IfcProxy, 8, IfcSchema::Type::IfcElement, 7); // IfcProxy.Tag & IfcElement.Tag

struct geom_filter {
    geom_filter(bool include, bool traverse) : type(UNUSED), include(include), traverse(traverse) {}
    geom_filter() : type(UNUSED), include(false), traverse(false) {}
    enum filter_type { UNUSED, ENTITY_TYPE, LAYER_NAME, ENTITY_ARG };
    filter_type type;
    bool include;
    bool traverse;
    string arg;
    set<string> values;
};

vector<IfcGeom::filter_t> setup_filters(const vector<geom_filter>& filters, const string& output_extension) {
    vector<IfcGeom::filter_t> filter_funcs;
    for (auto& f : filters) {
        if (f.type == geom_filter::ENTITY_TYPE) {
            entity_filter.include = f.include;
            entity_filter.traverse = f.traverse;
            try {
                entity_filter.populate(f.values);
            } catch (const IfcParse::IfcException& e) {
                cerr << "[Error] " << e.what() << endl;
                return vector<IfcGeom::filter_t>();
            }
        } else if (f.type == geom_filter::LAYER_NAME) {
            layer_filter.include = f.include;
            layer_filter.traverse = f.traverse;
            layer_filter.populate(f.values);
        } else if (f.type == geom_filter::ENTITY_ARG) {
            if (f.arg == GUID_ARG) {
                guid_filter.include = f.include;
                guid_filter.traverse = f.traverse;
                guid_filter.populate(f.values);
            } else if (f.arg == NAME_ARG) {
                name_filter.include = f.include;
                name_filter.traverse = f.traverse;
                name_filter.populate(f.values);
            } else if (f.arg == DESC_ARG) {
                desc_filter.include = f.include;
                desc_filter.traverse = f.traverse;
                desc_filter.populate(f.values);
            } else if (f.arg == TAG_ARG) {
                tag_filter.include = f.include;
                tag_filter.traverse = f.traverse;
                tag_filter.populate(f.values);
            }
        }
    }

    // If no entity names are specified these are the defaults to skip from output
    if (entity_filter.values.empty()) {
        try {
            set<string> entities;
            entities.insert("IfcSpace");
            if (output_extension == ".svg") {
                entity_filter.include = true;
            } else {
                entities.insert("IfcOpeningElement");
            }
            entity_filter.populate(entities);
        } catch (const IfcParse::IfcException& e) {
            cerr << "[Error] " << e.what() << endl;
            return vector<IfcGeom::filter_t>();
        }
    }

    if (!layer_filter.values.empty()) { filter_funcs.push_back(boost::ref(layer_filter));  }
    if (!entity_filter.values.empty()) { filter_funcs.push_back(boost::ref(entity_filter)); }
    if (!guid_filter.values.empty()) { filter_funcs.push_back(boost::ref(guid_filter)); }
    if (!name_filter.values.empty()) { filter_funcs.push_back(boost::ref(name_filter)); }
    if (!desc_filter.values.empty()) { filter_funcs.push_back(boost::ref(desc_filter)); }
    if (!tag_filter.values.empty()) { filter_funcs.push_back(boost::ref(tag_filter)); }

    return filter_funcs;
}

class IFCLoader {
    private:
        //Color3f defaultColor = Color3f(1,1,1);
        Color3f defaultColor = Color3f(1,0,1);
        IfcGeom::Kernel kernel;

        VROntologyPtr ontology;

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

    public:
        IFCLoader() {
            static VROntologyPtr o = 0;
            if (!o) {
                o = VROntology::create("BIM");
                VROntology::library["BIM"] = o;
                auto BIMelement = o->addConcept("BIMelement");
                BIMelement->addProperty("meta", "string");
            }

            ontology = o;
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

        void addSemantics(VRGeometryPtr obj, IfcGeom::Element<real_t>* ifc) {
            string type = ifc->type();
            auto e = ontology->addEntity(obj->getName(), "BIMelement");

            IfcRelDefines::list::ptr propertyList = ifc->product()->IsDefinedBy();
            for (IfcRelDefines* p : *propertyList) {
                IfcRelDefinesByProperties* by_prop = p->as<IfcRelDefinesByProperties>();
                if (by_prop) {
                    auto pdef = by_prop->RelatingPropertyDefinition();
                    if (IfcPropertySet* pset = pdef->as<IfcPropertySet>()) {
                        auto props = pset->HasProperties();

                        for (IfcProperty* prop : *props) {
                            map<string, string> paramData;
                            int c = prop->getArgumentCount();
                            for (int i = 0; i < c; i++) {
                                string name = p->getArgumentName(i);
                                Argument* arg = prop->getArgument(i);
                                paramData[name] = argVal(arg);
                                //e->add("meta", name+": "+paramData[name]);
                            }

                            if (paramData.count("GlobalId") && paramData.count("Name")) {
                                string n = paramData["GlobalId"];
                                string v = paramData["Name"];
                                e->add("meta", n+": "+v);
                            }
                        }
                    } else if(IfcElementQuantity* pset = pdef->as<IfcElementQuantity>()) { // TODO
                        cout << "AAA\n";
                        auto props = pset->Quantities();

                        for (IfcPhysicalQuantity* prop : *props) {
                            map<string, string> paramData;
                            int c = prop->getArgumentCount();
                            for (int i = 0; i < c; i++) {
                                string name = p->getArgumentName(i);
                                Argument* arg = prop->getArgument(i);
                                paramData[name] = argVal(arg);
                                e->add("meta", "AA: "+name+": "+paramData[name]);
                            }

                            /*if (paramData.count("GlobalId") && paramData.count("Name")) {
                                string n = paramData["GlobalId"];
                                string v = paramData["Name"];
                                e->add("meta", n+": "+v);
                            }*/
                        }
                    }
                }
            }

            obj->setEntity(e);
        }

        VRGeometryPtr convertGeo(const IfcGeom::TriangulationElement<float>* o) {
            if (!o) return 0;
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
            settings.set(IfcGeom::IteratorSettings::SEARCH_FLOOR,                 false);
            settings.set(IfcGeom::IteratorSettings::SITE_LOCAL_PLACEMENT,         false);
            settings.set(IfcGeom::IteratorSettings::BUILDING_LOCAL_PLACEMENT,     false);

            IfcGeom::BRepElement<float>* element = kernel.create_brep_for_representation_and_product<float>(settings, representation, product);
            return element;
		}

        IfcGeom::TriangulationElement<float>* createMesh(IfcGeom::BRepElement<float>* brep) {
			IfcGeom::TriangulationElement<float>* triangulation = 0;
			if (brep) triangulation = new IfcGeom::TriangulationElement<float>(*brep);
			return triangulation;
		}

        void load(string path, VRTransformPtr res) {
            VRObjectPtr root = VRObject::create(path);

            //Logger::SetOutput(&cout, &cout); // Redirect the output (both progress and log) to stdout
            Logger::SetOutput(0, 0); // Redirect the output (both progress and log) to stdout

            // Parse the IFC file provided in argv[1]
            IfcParse::IfcFile file;
            if ( !file.Init(path) ) { cout << "Unable to parse .ifc file: " << path << endl; return; }

            //  see IfcGeomIterator.h
            // IfcGeom::Iterator<real_t> context_iterator(settings, &file, filter_funcs);
            // context_iterator.initialize()
            // IfcGeom::Element<real_t>* geom_object = context_iterator.get();
            // auto geo = convertGeo(static_cast<const IfcGeom::TriangulationElement<real_t>*>(geom_object));

            auto elements = file.entitiesByType<IfcProduct>();
            cout << "Found " << elements->size() << " elements in " << path << ":" << endl;

            struct Ifc_Node {
                IfcProduct* element = 0;
                Ifc_Node* parent = 0;
                vector<Ifc_Node*> children;

                Ifc_Node() {}
                Ifc_Node(IfcProduct* e) : element(e) {}
            };

            map<IfcProduct*, Ifc_Node> nodes;
            for ( IfcProduct* element : *elements ) nodes[element] = Ifc_Node(element);

            for ( auto& n : nodes) {
                IfcProduct* e = n.first;
                string name = "UNNAMED";
                if (e->hasName()) name = e->Name();
                cout << name << ", " << e->GlobalId() << ", " << Type::ToString(e->type()) << endl;
                if (e->hasObjectPlacement()) {
                    gp_Trsf trsf;
                    kernel.convert(e->ObjectPlacement(), trsf);
                    const gp_XYZ& pos = trsf.TranslationPart();
                    cout << " pos: " << pos.X() << ", " << pos.Y() << ", " << pos.Z() << endl;
                }

                if (e->hasRepresentation()) {
                    IfcProductRepresentation* prod_rep = e->Representation();
                    if (prod_rep) { // IfcProductDefinitionShape
                        cout << "  prod_rep: " <<  Type::ToString(prod_rep->type()) << endl;
                        auto reps = prod_rep->Representations();
                        for (auto rep : *reps) { // IfcShapeRepresentation
                            if (rep) {
                                auto ifcBRep = createBRep(rep, e);
                                auto ifcMesh = createMesh(ifcBRep);
                                auto geo = convertGeo(ifcMesh);
                                if (geo) root->addChild(geo);
                                cout << "   rep: " << Type::ToString(rep->type()) << ", " << ifcBRep << ", " << ifcMesh << ", " << geo << endl;
                            }
                        }
                    }
                }


                /*for (int i=0; i<e->getArgumentCount(); i++) {
                    auto an = e->getArgumentName(i);
                    if (an) cout << " a" << i << ", " << an << endl;
                }*/
                //cout << " el " << e->Name() << ", " << e->Parent() << endl;
            }

            res->addChild(root);
        }
};

void OSG::loadIFC(string path, VRTransformPtr res) {
   IFCLoader ifc;
   ifc.load(path, res);

}

#endif
