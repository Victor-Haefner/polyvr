#include "VRSTEPCascade.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/material/VRMaterial.h"
#include "core/utils/toString.h"
#include "core/utils/VRProgress.h"

#include <StepBasic_SiUnitAndLengthUnit.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ColorTool.hxx>
#include <XCAFDoc_MaterialTool.hxx>
#include <XCAFDoc.hxx>
#include <STEPControl_Reader.hxx>
#include <STEPCAFControl_Reader.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_CompSolid.hxx>
#include <TopoDS_Solid.hxx>
#include <TopoDS_Shell.hxx>
#include <BRepTools.hxx>
#include <TopLoc_Location.hxx>
#include <TColgp_Array1OfPnt2d.hxx>
#include <Poly_Triangulation.hxx>
#include <Quantity_Color.hxx>
#include <TShort_Array1OfShortReal.hxx>
#include <BRepMesh_FastDiscret.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepLProp_SLProps.hxx>
#include <BRepGProp_Face.hxx>
#include <TopExp_Explorer.hxx>
#include <XCAFApp_Application.hxx>
#include <StepData_StepModel.hxx>
#include <TCollection_HAsciiString.hxx>
#include <TDocStd_Document.hxx>
#include <TDataStd_Name.hxx>
#include <TDF_LabelSequence.hxx>
#include <XSControl_WorkSession.hxx>

using namespace std;
using namespace OSG;

typedef double real_t;

VRProgress progress;
int lastStage = 0;

void on_update(int i, int N, int stage) {
    return;

    if (stage != lastStage) {
        cout << endl;
        progress.setup("STEP mesher stage " + toString(stage), N);
        progress.reset();
    }

    progress.update(i);
    //cout << "mesher update: " << i << "/" << N << " of stage " << stage << endl;
    lastStage = stage;
}

class STEPLoader {
    private:
        map<string, string> options;
        Handle(XCAFDoc_ColorTool) colors;
        Handle(XCAFDoc_MaterialTool) materials;

        pair<bool, Color3f> getColor(const TDF_Label& label) {
            bool valid = false;
            Quantity_Color c;
            if (!valid) valid = colors->GetColor(label, XCAFDoc_ColorSurf, c);
            if (!valid) valid = colors->GetColor(label, XCAFDoc_ColorCurv, c);
            if (!valid) valid = colors->GetColor(label, XCAFDoc_ColorGen , c);
            return make_pair(valid, Color3f(c.Red(), c.Green(), c.Blue()));
        }

        pair<bool, Color3f> getColor(const TopoDS_Shape& shape) {
            bool valid = false;
            Quantity_Color c;
            if (!valid) valid = colors->GetColor(shape, XCAFDoc_ColorSurf, c);
            if (!valid) valid = colors->GetColor(shape, XCAFDoc_ColorCurv, c);
            if (!valid) valid = colors->GetColor(shape, XCAFDoc_ColorGen , c);
            return make_pair(valid, Color3f(c.Red(), c.Green(), c.Blue()));
        }

        bool needsVertexColors(const TopoDS_Shape& shape) {
            for (TopExp_Explorer exp(shape, TopAbs_COMPOUND); exp.More(); exp.Next()) {
                const TopoDS_Compound& body = TopoDS::Compound(exp.Current());
                auto color = getColor(body);
                if (color.first) return true;
            }

            for (TopExp_Explorer exp(shape, TopAbs_COMPSOLID); exp.More(); exp.Next()) {
                const TopoDS_CompSolid& body = TopoDS::CompSolid(exp.Current());
                auto color = getColor(body);
                if (color.first) return true;
            }

            for (TopExp_Explorer exp(shape, TopAbs_SOLID); exp.More(); exp.Next()) {
                const TopoDS_Solid& body = TopoDS::Solid(exp.Current());
                auto color = getColor(body);
                if (color.first) return true;
            }

            for (TopExp_Explorer exp(shape, TopAbs_SHELL); exp.More(); exp.Next()) {
                const TopoDS_Shell& shell = TopoDS::Shell(exp.Current());
                auto color = getColor(shell);
                if (color.first) return true;
            }

            for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next()) {
                const TopoDS_Face& face = TopoDS::Face(exp.Current());
                auto color = getColor(face);
                if (color.first) return true;
            }
            return false;
        }

        void handleFace(const TopoDS_Face& face, VRGeoData& data, bool useVertexColors, Color3f color) {
            TopLoc_Location loc;
            Handle(Poly_Triangulation) tri = BRep_Tool::Triangulation(face, loc);
            if (tri.IsNull()) return;

            const TColgp_Array1OfPnt& nodes = tri->Nodes();
            int i0 = data.size();

            // face vertices
            for (int i = 1; i <= nodes.Length(); ++i) {
                gp_Pnt pnt = nodes(i).Transformed(loc);
                Pnt3d pos(pnt.X(), pnt.Y(), pnt.Z());
                data.pushPos( pos );
            }

            // face normals
            if (tri->HasUVNodes()) {
                const TColgp_Array1OfPnt2d& uvs = tri->UVNodes();
                BRepGProp_Face prop(face);
                gp_Vec n;
                gp_Pnt pnt;
                for (int i=1; i<=uvs.Length(); i++) {
                    gp_Pnt2d uv = uvs(i);
                    prop.Normal(uv.X(),uv.Y(),pnt,n);
                    Vec3d norm(n.X(), n.Y(), n.Z());
                    data.pushNorm( norm );
                }
            }

            // face triangle indices
            const Poly_Array1OfTriangle& triangles = tri->Triangles();
            for (int i = 1; i <= triangles.Length(); ++i) {
                int n1, n2, n3;
                triangles(i).Get(n1, n2, n3);
                if (face.Orientation() == TopAbs_REVERSED) data.pushTri(i0+n1-1, i0+n3-1, i0+n2-1);
                else                                       data.pushTri(i0+n1-1, i0+n2-1, i0+n3-1);
            }

            // face colors
            if (useVertexColors) {
                auto faceC = getColor(face);
                if (!faceC.first) faceC.second = color;//Color3f(0.5,0.9,0.4);
                for (int i = 1; i <= nodes.Length(); ++i) data.pushColor( faceC.second );
            }
        }

        void iterateFaces(const TopoDS_Shape& shape, VRGeoData& data, bool useVertexColors, Color3f color) {
            for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next()) {
                const TopoDS_Face& face = TopoDS::Face(exp.Current());
                handleFace(face, data, useVertexColors, color);
            }
        }

        string getTypeName(const TopoDS_Shape& shape) {
            TopAbs_ShapeEnum t = shape.ShapeType();
            if (t == TopAbs_SHAPE) return "shape";
            if (t == TopAbs_COMPOUND) return "compound";
            if (t == TopAbs_COMPSOLID) return "compsolid";
            if (t == TopAbs_SOLID) return "solid";
            if (t == TopAbs_SHELL) return "shell";
            if (t == TopAbs_FACE) return "face";
            if (t == TopAbs_WIRE) return "wire";
            if (t == TopAbs_EDGE) return "edge";
            if (t == TopAbs_VERTEX) return "vertex";
            return "shape";
        }

        void iterateShell(const TopoDS_Shape& shell, VRTransformPtr parent, VRGeoData* pdata, bool useVertexColors, Color3f color, bool verbose = false) {
            auto colShell = getColor(shell);
            if (colShell.first) color = colShell.second;
            if (verbose) cout << endl << " ----- traverse shell: " << getTypeName(shell) << endl;

            if (!pdata) {
                VRGeoData data;
                iterateFaces(shell, data, useVertexColors, color);
                auto geo = data.asGeometry("shell");
                parent->addChild(geo);
            } else {
                iterateFaces(shell, *pdata, useVertexColors, color);
            }
        }

        void iterateFace(const TopoDS_Shape& faceS, VRTransformPtr parent, VRGeoData* pdata, bool useVertexColors, Color3f color, bool verbose = false) {
            auto colFace = getColor(faceS);
            if (colFace.first) color = colFace.second;
            if (verbose) cout << endl << " ----- traverse face: " << getTypeName(faceS) << endl;

            const TopoDS_Face& face = TopoDS::Face(faceS);
            if (!pdata) {
                VRGeoData data;
                handleFace(face, data, useVertexColors, color);
                auto geo = data.asGeometry("face");
                parent->addChild(geo);
            } else {
                handleFace(face, *pdata, useVertexColors, color);
            }
        }

        VRTransformPtr iterateShape(const TopoDS_Shape& shape, bool useVertexColors, Color3f color, VRTransformPtr parent = 0, VRGeoData* data = 0, bool verbose = false) {
            auto colShape = getColor(shape);
            if (colShape.first) color = colShape.second;
            if (verbose) cout << " ----- traverse shape: " << getTypeName(shape) << endl;

            VRTransformPtr obj;
            if (!data) obj = VRTransform::create(getTypeName(shape));
            TopoDS_Iterator sIter(shape);

            do {
                TopoDS_Shape subShape;
                TopAbs_ShapeEnum t;

                try {
                    subShape = sIter.Value();
                    sIter.Next();
                    t = subShape.ShapeType();
                }
                catch(Standard_Failure& e) { break; } // either invalid value or no more value
                catch(exception& e) { cout << endl << "ERROR: unexpected exception " << e.what() << endl; break; }
                catch(...) {
                    cout << " Warning in STEP convertGeo: unknown exception" << endl;
                    std::exception_ptr p = std::current_exception();
                    cout << (p ? p.__cxa_exception_type()->name() : "null") << endl;
                    break;
                }

                //static int testN=0;
                //testN++;
                //cout << "  teestN " << testN << endl;

                if (t == TopAbs_SHAPE) iterateShape(subShape, useVertexColors, color, obj, data, verbose);
                if (t == TopAbs_COMPOUND) iterateShape(subShape, useVertexColors, color, obj, data, verbose);
                if (t == TopAbs_COMPSOLID) iterateShape(subShape, useVertexColors, color, obj, data, verbose);
                if (t == TopAbs_SOLID) iterateShape(subShape, useVertexColors, color, obj, data, verbose);
                if (t == TopAbs_SHELL) iterateShell(subShape, obj, data, useVertexColors, color, verbose);
                if (t == TopAbs_FACE) iterateFace(subShape, obj, data, useVertexColors, color, verbose);
                if (t == TopAbs_EDGE) continue; // ignore edges
                if (t == TopAbs_WIRE || t == TopAbs_VERTEX) {
                    cout << "ERROR! unexpected shape types while iterating: " << getTypeName(subShape) << endl;
                }
            } while(sIter.More());

            if (parent && obj) {
                if (obj->getChildrenCount() > 0) parent->addChild(obj);
            }
            return obj;
        }

        VRTransformPtr convertGeo(const TopoDS_Shape& shape, bool subParts = false, bool verbose = false) {
            if (shape.IsNull()) return 0;

            double linear_deflection = 0.1;
            double angular_deflection = 0.5;
            //cout << "step convert shape dim max: " << Dmax << ", ld: " << linear_deflection << endl;

            //BRepMesh_IncrementalMesh mesher(shape, linear_deflection, false, angular_deflection, true); // shape, linear deflection, relative to edge length, angular deflection, paralellize
            try {
                BRepMesh_IncrementalMesh mesher(shape, linear_deflection, true, angular_deflection, true, on_update); // shape, linear deflection, relative to edge length, angular deflection, paralellize
            }
            catch(exception& e) { cout << " Warning in STEP convertGeo: " << e.what() << endl;  return 0; }
            catch(...) { cout << " Warning in STEP convertGeo: unknown exception" << endl; return 0; }

            bool useVertexColors = needsVertexColors(shape);

            Color3f colDefault(0.5,0.9,0.4);
            if (subParts) return iterateShape(shape, useVertexColors, colDefault);
            else {
                VRGeoData data;
                iterateShape(shape, useVertexColors, colDefault, 0, &data, verbose);
                if (data.size() == 0) return 0;
                return data.asGeometry("part");
            }
        }

        void applyMaterial(VRGeometryPtr geo, const TopoDS_Shape& shape) {
            auto mat = VRMaterial::create("mat");
            geo->setMaterial(mat);
            auto color = getColor(shape);
            if (color.first) mat->setDiffuse(color.second);
            else mat->setDiffuse(Color3f(0.5,0.7,0.9));
        }

        float getScale(STEPCAFControl_Reader& reader) {
            const STEPControl_Reader& stpreader = reader.Reader();
            const Handle(Interface_InterfaceModel) Model = stpreader.Model();
            Handle(StepData_StepModel) aSM = Handle(StepData_StepModel)::DownCast(Model);

            for (int i=1; i<=Model->NbEntities(); i++) {
                Handle(Standard_Transient) enti = aSM->Entity(i);

                if (enti->IsKind(STANDARD_TYPE(StepBasic_SiUnitAndLengthUnit))) {
                    Handle(StepBasic_SiUnitAndLengthUnit) unit = Handle(StepBasic_SiUnitAndLengthUnit)::DownCast(enti);

                    if (unit->HasPrefix() && (StepBasic_SiUnitName::StepBasic_sunMetre == unit->Name()) && (StepBasic_SiPrefix::StepBasic_spMilli == unit->Prefix())) {
                        return 0.001; // millimeter
                    } else if (!unit->HasPrefix() && (StepBasic_SiUnitName::StepBasic_sunMetre == unit->Name())) {
                        return 1.0; // meter
                    } else {
                        return 1.0; // unknown
                    }
                }
            }

            return 1.0;
        }

    public:
        STEPLoader() {}

        string toString(Handle(TCollection_HAsciiString)& s) { return string(s->ToCString()); }

        string toString(Standard_GUID id) {
            string name = "";
            name.resize(36);
            char* buffer = &name[0];
            id.ToCString(buffer);
            return name;
        }

        string toString(Handle(TDataStd_Name)& N) {
            string name = "";
            name.resize(N->Get().LengthOfCString());
            char* buffer = &name[0];
            N->Get().ToUTF8CString( buffer );
            return name;
        }

        string getName(TDF_Label& label) {
            Handle(TDataStd_Name) N;
            if ( label.FindAttribute(TDataStd_Name::GetID(),N)) return toString(N);
            return "UNKNOWN";
        }

        void load(string path, VRTransformPtr res, map<string, string> opts) {
            options = opts;

            try {
                Handle(TDocStd_Document) aDoc;
                Handle(XCAFApp_Application) anApp = XCAFApp_Application::GetApplication();
                anApp->NewDocument("MDTV-XCAF",aDoc);

                STEPCAFControl_Reader reader(on_update);
                auto status = reader.ReadFile(path.c_str());
                if (!status) { cout << "failed to read file" << endl; return; }
                int Nroots = reader.NbRootsForTransfer();
                cout << "Number of roots in STEP file: " << Nroots << endl;
                reader.SetNameMode(true);
                reader.SetMatMode(true);
                reader.SetColorMode(true);
                reader.SetLayerMode(true);

                float scale = getScale(reader);
                cout << "Model length unit scale: " << scale << endl;

                int countTransfers = 0;
                for (int i=1; i<=Nroots; i++) {
                    cout << " transfer " << i << "/" << Nroots << endl;
                    auto transferOk = reader.TransferOneRoot(i, aDoc);
                    cout << endl;
                    if (!transferOk) cout << "failed to transfer to XDS doc" << endl;
                    else countTransfers++;
                }

                if (countTransfers < Nroots) cout << "Warning! failed to transfer some roots, model might be incomplete!" << endl;
                if (countTransfers == 0) { cout << "Error! failed to transfer any root" << endl; return; }
                cout << "XCAF transfers done " << endl;


                Handle(XCAFDoc_ShapeTool) Assembly = XCAFDoc_DocumentTool::ShapeTool(aDoc->Main());
                TDF_LabelSequence shapes;
                TDF_LabelSequence rootShapes;
                Assembly->GetShapes(shapes);
                Assembly->GetFreeShapes(rootShapes);
                cout << "found " << shapes.Length() << " shapes, and " << rootShapes.Length() << " root shapes" << endl;


                colors = XCAFDoc_DocumentTool::ColorTool(aDoc->Main());
                TDF_LabelSequence cols;
                colors->GetColors(cols);
                cout << "imported colors: (" << cols.Length() << ")" << endl;
                for (int i=1; i<=cols.Length(); i++) cout << " color: " << getColor( cols.Value(i) ).second << " " << getColor( cols.Value(i) ).first << endl;
                //for (int i=1; i<=cols.Length(); i++) cout << " color: " << getColor( shapes.Value(i) ).second << " " << getColor( shapes.Value(i) ).first << endl;

                materials = XCAFDoc_DocumentTool::MaterialTool( aDoc->Main() );
                TDF_LabelSequence mats;
                materials->GetMaterialLabels(mats);
                cout << "imported materials: (" << mats.Length() << ")" << endl;

                bool subParts = false;
                if (options.count("subParts")) subParts = true;

                cout << "build STEP parts:" << endl;
                map<int, VRTransformPtr> parts;
                for (int i=1; i<=shapes.Length(); i++) {
                    TopoDS_Shape shape = Assembly->GetShape(shapes.Value(i));
                    TDF_Label label = Assembly->FindShape(shape, false);
                    //cout << " shape label:" << endl << label << endl;
                    if ( (!label.IsNull()) && (Assembly->IsShape(label)) ) {
                        string name = getName(label);
                        //cout << "  shape " << name << " " << Assembly->IsSimpleShape(label) << " " << Assembly->IsAssembly(label) << " " << Assembly->IsFree(label) << endl;
                        if (Assembly->IsSimpleShape(label)) {
                            //cout << " create shape " << name << endl;
                            VRTransformPtr obj = convertGeo(shape, subParts);
                            if (obj) {
                                obj->setName( name );
                                if (parts.count(label.Tag())) cout << "Warning in STEP import, the label tag " << label.Tag() << " is allready used!" << endl;
                                parts[label.Tag()] = obj;

                                if (!subParts) {
                                    VRGeometryPtr geo = dynamic_pointer_cast<VRGeometry>(obj);
                                    if (geo) {
                                        applyMaterial(geo, shape);
                                        geo->setReference(VRGeometry::Reference(VRGeometry::FILE, path+"|"+geo->getName()));
                                    }
                                } else {
                                    for (auto c : obj->getChildren(true, "Geometry")) {
                                        VRGeometryPtr geo = dynamic_pointer_cast<VRGeometry>(c);
                                        if (geo) {
                                            applyMaterial(geo, shape);
                                            geo->setReference(VRGeometry::Reference(VRGeometry::FILE, path+"|"+geo->getName()));
                                        }
                                    }
                                }
                            } else {
                                cout << "Warning in STEP import, no geometry for shape '" << name << "'" << endl;
                            }
                        }
                    }
                }

                auto applyTransform = [&](VRTransformPtr obj, TopoDS_Shape& shape) {
                    TopLoc_Location l = shape.Location();
                    gp_Trsf c = l.Transformation();
                    gp_XYZ t = c.TranslationPart();
                    gp_Mat m = c.VectorialPart();
                    Matrix4d mat = Matrix4d(
                        m(1,1)  ,m(1,2) ,m(1,3) ,t.X(),
                        m(2,1)  ,m(2,2) ,m(2,3) ,t.Y(),
                        m(3,1)  ,m(3,2) ,m(3,3) ,t.Z(),
                        0       ,0      ,0      ,1
                        );
                    obj->setMatrix(mat);
                };

                cout << "build STEP assembly" << endl;
                function<void (TDF_Label, VRTransformPtr)> explore = [&](TDF_Label node, VRTransformPtr parent) {
                    TopoDS_Shape shape = Assembly->GetShape(node);
                    TDF_Label    label = Assembly->FindShape(shape, false);

                    string name = getName(label);
                    //cout << " add node: " << name << " ID: " << label.Tag() << endl;

                    if (Assembly->IsSimpleShape(label)) {
                        if (parts.count( label.Tag() )) {
                            auto partTemplate = parts[label.Tag()];
                            if (partTemplate) {
                                VRTransformPtr part = dynamic_pointer_cast<VRTransform>( partTemplate->duplicate() );
                                if (part) {
                                    applyTransform(part, shape);
                                    parent->addChild( part );
                                }
                            }
                        }
                    }

                    if (Assembly->IsAssembly(label)) {
                        TDF_LabelSequence children;
                        Assembly->GetComponents(label, children, false);
                        VRTransformPtr t = VRTransform::create(name);
                        applyTransform(t, shape);
                        parent->addChild(t);
                        for (int i=1; i<=children.Length(); i++) explore(children.Value(i), t);
                    }
                };

                // explore root shape
                for (int i=1; i<=rootShapes.Length(); i++) explore(rootShapes.Value(i), res);

                for (auto c : res->getChildren()) {
                    auto t = dynamic_pointer_cast<VRTransform>(c);
                    if (t) t->setScale(Vec3d(scale, scale, scale));
                }
            } catch(exception e) {
                cout << " STEP import failed in load: " << e.what() << endl;
            }
        }
};

void OSG::loadSTEPCascade(string path, VRTransformPtr res, map<string, string> options) {
    STEPLoader step;
    step.load(path, res, options);
}
