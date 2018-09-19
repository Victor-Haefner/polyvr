#include "VRSTEPCascade.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/material/VRMaterial.h"
#include "core/utils/toString.h"
#include "core/utils/VRProgress.h"

#include <XCAFDoc_ShapeTool.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ColorTool.hxx>
#include <XCAFDoc.hxx>
#include <STEPControl_Reader.hxx>
#include <STEPCAFControl_Reader.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_CompSolid.hxx>
#include <TopoDS_Solid.hxx>
#include <TopoDS_Shell.hxx>
#include <BRepTools.hxx>
#include <TopLoc_Location.hxx>
#include <Poly_Triangulation.hxx>
#include <Quantity_Color.hxx>
#include <TShort_Array1OfShortReal.hxx>
#include <BRepMesh_FastDiscret.hxx>
#include <BRepLProp_SLProps.hxx>
#include <XCAFApp_Application.hxx>
#include <StepData_StepModel.hxx>
#include <TCollection_HAsciiString.hxx>
#include <TDocStd_Document.hxx>
#include <TDataStd_Name.hxx>
#include <TDF_LabelSequence.hxx>
#include <XSControl_WorkSession.hxx>

#include <ifcgeom/IfcGeom.h>

using namespace std;
using namespace OSG;
using namespace IfcSchema;

typedef double real_t;

VRProgress progress;
int lastStage = 0;

void on_update(int i, int N, int stage) {
    if (stage != lastStage) {
        cout << "next stage " << stage << endl;
        progress.setup("STEP mesher stage " + toString(stage), N);
        progress.reset();
    }

    progress.update(i);
    //cout << "mesher update: " << i << "/" << N << " of stage " << stage << endl;
    lastStage = stage;
}

class STEPLoader {
    private:
        VRGeometryPtr convertGeo(const TopoDS_Shape& shape) {
            if (shape.IsNull()) return 0;

            float linear_deflection = 0.1;
            float angular_deflection = 0.5;

            BRepMesh_IncrementalMesh mesher(shape, linear_deflection, false, angular_deflection, true); // shape, linear deflection, relative to edge length, angular deflection, paralellize
            //BRepMesh_IncrementalMesh mesher(shape, linear_deflection, false, angular_deflection, true, on_update); // shape, linear deflection, relative to edge length, angular deflection, paralellize
            VRGeoData data;

            for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next()) {
                cout << "STEPLoader::convertGeo, data size: " << data.size() << endl;
                const TopoDS_Face& face = TopoDS::Face(exp.Current());
                TopLoc_Location loc;
                Handle(Poly_Triangulation) tri = BRep_Tool::Triangulation(face, loc);
                if (tri.IsNull()) continue;

                const TColgp_Array1OfPnt& nodes = tri->Nodes();
                int i0 = data.size();

                for (int i = 1; i <= nodes.Length(); ++i) {
                    gp_Pnt pnt = nodes(i).Transformed(loc);
                    Pnt3d pos(pnt.X(), pnt.Y(), pnt.Z());
                    data.pushPos( pos );
                }

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

                const Poly_Array1OfTriangle& triangles = tri->Triangles();
                for (int i = 1; i <= triangles.Length(); ++i) {
                    int n1, n2, n3;
                    triangles(i).Get(n1, n2, n3);
                    if (face.Orientation() == TopAbs_REVERSED) data.pushTri(i0+n1-1, i0+n3-1, i0+n2-1);
                    else                                       data.pushTri(i0+n1-1, i0+n2-1, i0+n3-1);
                }
            }

            string name = "test"; //shape.Name();
            auto geo = data.asGeometry(name);
            //applyTransformation(o,geo);
            return geo;
        }

    public:
        STEPLoader() {}

        string toString(Handle(TCollection_HAsciiString)& s) {
            return string(s->ToCString());
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
            Handle(TDocStd_Document) aDoc;
            Handle(XCAFApp_Application) anApp = XCAFApp_Application::GetApplication();
            anApp->NewDocument("MDTV-XCAF",aDoc);

            STEPCAFControl_Reader reader;
            IFSelect_ReturnStatus stat = reader.ReadFile(path.c_str());
            cout << "Number of roots in STEP file: " << reader.NbRootsForTransfer() << endl;
            reader.SetNameMode(true);
            //reader.SetMatMode(true);
            reader.SetColorMode(true);
            reader.SetLayerMode(true);
            auto transferOk = reader.Transfer(aDoc);
            if (!transferOk) { cout << "failed to transfer to XDS doc" << endl; return; }
            cout << "XCAF transfer ok " << endl;

            Handle(XCAFDoc_ShapeTool) Assembly = XCAFDoc_DocumentTool::ShapeTool(aDoc->Main());
            TDF_LabelSequence shapes;
            TDF_LabelSequence rootShapes;
            Assembly->GetShapes(shapes);
            Assembly->GetFreeShapes(rootShapes);
            cout << "found " << shapes.Length() << " shapes, and " << rootShapes.Length() << " root shapes" << endl;


            Handle(XCAFDoc_ColorTool) colors = XCAFDoc_DocumentTool::ColorTool(aDoc->Main());
            Quantity_Color c;

            map<string, VRGeometryPtr> parts;
            for (int i=1; i<=shapes.Length(); i++) {
                TopoDS_Shape shape = Assembly->GetShape(shapes.Value(i));
                TDF_Label aLabel = Assembly->FindShape(shape, false);
                if ( (!aLabel.IsNull()) && (Assembly->IsShape(aLabel)) ) {
                    string name = getName(aLabel);
                    cout << " shape " << name << " " << Assembly->IsSimpleShape(aLabel) << " " << Assembly->IsAssembly(aLabel) << " " << Assembly->IsFree(aLabel) << endl;
                    if (Assembly->IsSimpleShape(aLabel)) {
                        cout << "  create shape\n";
                        auto obj = convertGeo(shape);
                        obj->setName( name );
                        if (parts.count(name)) cout << "Warning in STEP import, the name '" << name << "' is allready taken!" << endl;
                        parts[name] = obj;
                        auto mat = VRMaterial::create("mat");
                        obj->setMaterial(mat);
                        //colors->GetColor(shape, XCAFDoc_ColorGen, c);
                        //colors->GetColor(shape, XCAFDoc_ColorCurv, c);
                        colors->GetColor(shape, XCAFDoc_ColorSurf, c);
                        Color3f color = Color3f(c.Red(), c.Green(), c.Blue());
                        mat->setDiffuse(color);
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

            function<void (TDF_Label&, TopoDS_Shape&, VRTransformPtr)> explore = [&](TDF_Label& label, TopoDS_Shape& shape, VRTransformPtr parent) {
                string name = getName(label);

                if (Assembly->IsSimpleShape(label)) {
                    VRGeometryPtr part = dynamic_pointer_cast<VRGeometry>( parts[name]->duplicate() );
                    applyTransform(part, shape);
                    parent->addChild( part );
                }

                if (Assembly->IsAssembly(label)) {
                    TDF_LabelSequence children;
                    Assembly->GetComponents(label, children, false);
                    VRTransformPtr t = VRTransform::create(name);
                    applyTransform(t, shape);
                    parent->addChild(t);
                    for (int i=1; i<=children.Length(); i++) {
                        TopoDS_Shape cShape = Assembly->GetShape(children.Value(i));
                        TDF_Label cLabel = Assembly->FindShape(cShape, false);
                        explore(cLabel, cShape, t);
                    }
                }
            };

            // get root shape
            for (int i=1; i<=rootShapes.Length(); i++) {
                TopoDS_Shape shape = Assembly->GetShape(rootShapes.Value(i));
                TDF_Label aLabel = Assembly->FindShape(shape, false);
                explore(aLabel, shape, res);
            }
        }
};

void OSG::loadSTEPCascade(string path, VRTransformPtr res) {
    STEPLoader step;
    step.load(path, res);
}
