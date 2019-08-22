#include "VRSTEPCascade.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/material/VRMaterial.h"
#include "core/utils/toString.h"
#include "core/utils/VRProgress.h"

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

        VRGeometryPtr convertGeo(const TopoDS_Shape& shape) {
            if (shape.IsNull()) return 0;

            double linear_deflection = 0.1;
            double angular_deflection = 0.5;
            //cout << "step convert shape dim max: " << Dmax << ", ld: " << linear_deflection << endl;

            //BRepMesh_IncrementalMesh mesher(shape, linear_deflection, false, angular_deflection, true); // shape, linear deflection, relative to edge length, angular deflection, paralellize
            BRepMesh_IncrementalMesh mesher(shape, linear_deflection, true, angular_deflection, true, on_update); // shape, linear deflection, relative to edge length, angular deflection, paralellize
            VRGeoData data;

            auto shapeColor = getColor(shape);
            if (!shapeColor.first) shapeColor.second = Color3f(0.5,0.9,0.4);
            bool useVertexColors = false;
            for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next()) {
                const TopoDS_Face& face = TopoDS::Face(exp.Current());
                auto color = getColor(face);
                if (color.first) { useVertexColors = true; break; }
            }

            for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next()) {
                const TopoDS_Face& face = TopoDS::Face(exp.Current());
                TopLoc_Location loc;
                Handle(Poly_Triangulation) tri = BRep_Tool::Triangulation(face, loc);
                if (tri.IsNull()) continue;

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
                    auto color = getColor(face);
                    if (!color.first) color.second = shapeColor.second;
                    for (int i = 1; i <= nodes.Length(); ++i) data.pushColor( color.second );
                }
            }

            string name = "test"; //shape.Name();
            auto geo = data.asGeometry(name);
            //applyTransformation(o,geo);
            return geo;
        }

        void applyMaterial(VRGeometryPtr geo, const TopoDS_Shape& shape) {
            auto mat = VRMaterial::create("mat");
            geo->setMaterial(mat);
            auto color = getColor(shape);
            if (color.first) mat->setDiffuse(color.second);
            else mat->setDiffuse(Color3f(0.5,0.7,0.9));
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

        void load(string path, VRTransformPtr res) {
            try {
                Handle(TDocStd_Document) aDoc;
                Handle(XCAFApp_Application) anApp = XCAFApp_Application::GetApplication();
                anApp->NewDocument("MDTV-XCAF",aDoc);

                STEPCAFControl_Reader reader(on_update);
                auto status = reader.ReadFile(path.c_str());
                if (!status) { cout << "failed to read file" << endl; return; }
                cout << "Number of roots in STEP file: " << reader.NbRootsForTransfer() << endl;
                reader.SetNameMode(true);
                reader.SetMatMode(true);
                reader.SetColorMode(true);
                reader.SetLayerMode(true);
                auto transferOk = reader.Transfer(aDoc);
                cout << endl;
                if (!transferOk) { cout << "failed to transfer to XDS doc" << endl; return; }
                cout << "XCAF transfer ok " << endl;

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

                cout << "build STEP parts:" << endl;
                map<int, VRGeometryPtr> parts;
                for (int i=1; i<=shapes.Length(); i++) {
                    TopoDS_Shape shape = Assembly->GetShape(shapes.Value(i));
                    TDF_Label label = Assembly->FindShape(shape, false);
                    //cout << " shape label:" << endl << label << endl;
                    if ( (!label.IsNull()) && (Assembly->IsShape(label)) ) {
                        string name = getName(label);
                        //cout << "  shape " << name << " " << Assembly->IsSimpleShape(label) << " " << Assembly->IsAssembly(label) << " " << Assembly->IsFree(label) << endl;
                        if (Assembly->IsSimpleShape(label)) {
                            cout << " create shape " << name << endl;
                            VRGeometryPtr obj = convertGeo(shape);
                            obj->setName( name );
                            applyMaterial(obj, shape);
                            if (parts.count(label.Tag())) cout << "Warning in STEP import, the label tag " << label.Tag() << " is allready used!" << endl;
                            parts[label.Tag()] = obj;
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
                    cout << " add node: " << name << " ID: " << label.Tag() << endl;

                    if (Assembly->IsSimpleShape(label)) {
                        VRGeometryPtr part = dynamic_pointer_cast<VRGeometry>( parts[label.Tag()]->duplicate() );
                        applyTransform(part, shape);
                        parent->addChild( part );
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
            } catch(exception e) {
                cout << " STEP import failed in load: " << e.what() << endl;
            }
        }
};

void OSG::loadSTEPCascade(string path, VRTransformPtr res) {
    STEPLoader step;
    step.load(path, res);
}
