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
//#include <IGESControl_Controller.hxx>
//#include <Standard_Version.hxx>
//#include <boost/program_options.hpp>


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

        VROntologyPtr ontology;

        void applyTransformation(const IfcGeom::TriangulationElement<real_t>* o, VRGeometryPtr geo) {
            IfcGeom::Transformation<real_t> trans = o->transformation();
            const IfcGeom::Matrix<real_t>& m = trans.matrix();
            const vector<real_t> v = m.data();
            //Matrix4d osgM = Matrix4d(v[0], v[1], v[2], v[9], v[3], v[4], v[5], v[10], v[6], v[7], v[8], v[11], 0, 0, 0, 1);
            Matrix4d osgM = Matrix4d(v[0], v[3], v[6], v[9],
                                     v[1], v[4], v[7], v[10],
                                     v[2], v[5], v[8], v[11],
                                     0,    0,    0,    1);
            geo->setMatrix(osgM);
        }

        Color3f getColor(int faceID, vector<int>& mat_ids, vector<IfcGeom::Material>& materials) {
            if (faceID < 0 || faceID >= mat_ids.size()) return defaultColor;
            int matID = mat_ids[faceID];
            if (matID < 0 || matID >= materials.size()) return defaultColor;
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
                }
            }
            return "";
        }

        void addSemantics(VRGeometryPtr obj, IfcGeom::Element<real_t>* ifc) {
            string type = ifc->type();
            auto e = ontology->addEntity(obj->getName(), "BIMelement");

            IfcRelDefines::list::ptr propertyList = ifc->product()->IsDefinedBy();
            for (IfcRelDefines* p : *propertyList) {
                IfcRelDefinesByProperties* by_prop = 0;

                if (by_prop = p->as<IfcRelDefinesByProperties>()) {
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

        VRGeometryPtr convertGeo(const IfcGeom::TriangulationElement<real_t>* o) {
            const IfcGeom::Representation::Triangulation<real_t>& mesh = o->geometry();

            vector<real_t> verts = mesh.verts();
            vector<real_t> norms = mesh.normals();
            vector<int> faces = mesh.faces();
            vector<int> mat_ids = mesh.material_ids();
            vector<IfcGeom::Material> materials = mesh.materials();
            //vector<const IfcGeom::Element<float>*> parents = o->parents();

            if (faces.size() == 0) return 0;

            VRGeoData data;
            for (int i=0; i<verts.size(); i+=3) data.pushPos( Pnt3d(verts[i], verts[i+1], verts[i+2]) );
            for (int i=0; i<norms.size(); i+=3) data.pushNorm( Vec3d(norms[i], norms[i+1], norms[i+2]) );
            for (int i=0; i<faces.size(); i+=3) data.pushTri( faces[i], faces[i+1], faces[i+2] );

            vector<Color3f> cols = vector<Color3f>(verts.size()/3, defaultColor); // TODO: replace by material system!
            int Nfaces = faces.size()/3;
            //cout << "Mat " << Nfaces << " " << mat.original_name() << " " << color << endl;
            for (int i=0; i<Nfaces; i++) {
                Color3f color = getColor(i, mat_ids, materials);
                cols[faces[3*i  ]] = color;
                cols[faces[3*i+1]] = color;
                cols[faces[3*i+2]] = color;
            }
            for (int i=0; i<cols.size(); i++) data.pushColor( cols[i] );

            string name = o->name();// + "_" + o->unique_id();
            auto geo = data.asGeometry(name);
            applyTransformation(o,geo);
            return geo;
        }

        void load(string path, VRTransformPtr res) {
            //Logger::SetOutput(&cout, &cout); // Redirect the output (both progress and log) to stdout
            Logger::SetOutput(0, 0); // Redirect the output (both progress and log) to stdout

            // Parse the IFC file provided in argv[1]
            IfcParse::IfcFile file;
            if ( !file.Init(path) ) {
                cout << "Unable to parse .ifc file: " << path << endl;
                return;
            }

            // Lets get a list of IfcBuildingElements, this is the parent
            // type of things like walls, windows and doors.
            // entitiesByType is a templated function and returns a
            // templated class that behaves like a vector.
            // Note that the return types are all typedef'ed as members of
            // the generated classes, ::list for the templated vector class,
            // ::ptr for a shared pointer and ::it for an iterator.
            // We will simply iterate over the vector and print a string
            // representation of the entity to stdout.

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
            //settings.set_deflection_tolerance(1e-5);
            //settings.precision = 1e-6;

            string output_extension = ".dae";
            vector<geom_filter> used_filters;
            vector<IfcGeom::filter_t> filter_funcs = setup_filters(used_filters, output_extension);
            IfcGeom::Iterator<real_t> context_iterator(settings, &file, filter_funcs);
            if (!context_iterator.initialize()) { cout << "Failed to initialize IFC context_iterator"; return; }

            //serializer->setFile(context_iterator.getFile());
            //serializer->setUnitNameAndMagnitude("METER", 1.0f);

            int i=0;
            do {
                cout << "IFC process object " << i << endl;
                IfcGeom::Element<real_t>* geom_object = context_iterator.get();
                auto geo = convertGeo(static_cast<const IfcGeom::TriangulationElement<real_t>*>(geom_object));
                if (geo) {
                    res->addChild(geo);
                    addSemantics(geo, geom_object);
                }

                //const int progress = context_iterator.progress() / 2;
                //if (old_progress != progress) Logger::ProgressBar(progress);
                //old_progress = progress;
                i++;
                //if (i > 2) return;
            } while(context_iterator.next());

            /*auto elements = file.entitiesByType<IfcBuildingElement>();
            cout << "Found " << elements->size() << " elements in " << path << ":" << endl;

            for ( auto it = elements->begin(); it != elements->end(); ++ it ) {
                const IfcBuildingElement* element = *it;
                //cout << element->entity->toString() << endl;

                if ( element->is(IfcWindow::Class()) ) {
                    const IfcWindow* window = (IfcWindow*)element;

                    if ( window->hasOverallWidth() && window->hasOverallHeight() ) {
                        const double area = window->OverallWidth()*window->OverallHeight();
                        cout << "The area of this window is " << area << endl;
                    }
                }

            }*/

        }
};

void OSG::loadIFC(string path, VRTransformPtr res) {
    IFCLoader ifc;
    ifc.load(path, res);
}

#endif