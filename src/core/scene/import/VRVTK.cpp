#include "VRVTK.h"

#include <vtkVersion.h>
#include <vtkDataSetReader.h>
#include <vtkDataSet.h>
#include <vtkDataArray.h>
#include <vtkCellData.h>
#include <vtkPointData.h>
#include <vtkCell.h>
#include <vtkQuad.h>
#include <vtkLine.h>

/* NOT COMPILING?
sudo apt-get install libvtk9-dev
*/

#include <fstream>
#include "core/utils/toString.h"
#include "core/utils/VRProgress.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/material/VRMaterial.h"

OSG_BEGIN_NAMESPACE;

void loadVtk(string path, VRTransformPtr res) {
    cout << "load VTK file " << path << endl;
    VRGeoData geo;

    vtkDataSetReader* reader = vtkDataSetReader::New();
    reader->SetFileName(path.c_str());
    reader->ReadAllScalarsOn();
    reader->ReadAllVectorsOn();
    reader->ReadAllNormalsOn();
    reader->ReadAllTensorsOn();
    reader->ReadAllTCoordsOn();
    reader->ReadAllFieldsOn();
    reader->ReadAllColorScalarsOn();
    //reader->DebugOn();
    reader->Update();

    vtkDataSet* dataset = reader->GetOutput();

    int fVmaj = reader->GetFileMajorVersion();
    int fVmin = reader->GetFileMinorVersion();
    string fVer = toString(fVmaj) + "." + toString(fVmin);
    string vtkVer = vtkVersion::GetVTKVersion();
    cout << " vtk version: " << vtkVer << endl;
    cout << " file version: " << fVer << endl;


    int npoints = dataset->GetNumberOfPoints();
    int ncells = dataset->GetNumberOfCells();
    int nscalars = reader->GetNumberOfScalarsInFile();
    int nvectors = reader->GetNumberOfVectorsInFile();
    int ntensors = reader->GetNumberOfTensorsInFile();
    cout << " dataset sizes: " << npoints << " " << ncells << " " << nscalars << " " << nvectors << " " << ntensors << endl;

    for (int i=0; i<npoints; i++) {
        auto p = dataset->GetPoint(i);
        Vec3d v(p[0], p[1], p[2]);
        geo.pushVert(v);
        //cout << "point " << v << endl;
    }

    auto getCellPIDs = [](vtkCell* c) {
        vector<int> res;
        auto ids = c->GetPointIds();
        for (int k=0; k<ids->GetNumberOfIds(); k++) {
            res.push_back( ids->GetId(k) );
        }
        return res;
    };

    for (int i=0; i<ncells; i++) {
        vtkCell* c = dataset->GetCell(i);
        //int d = c->GetCellDimension();
        //int t = c->GetCellType();

        string type = c->GetClassName();
        if (type == "vtkQuad") {
            auto j = getCellPIDs(c); // size: 4
            geo.pushQuad(j[0], j[1], j[2], j[3]);
            continue;
        }

        if (type == "vtkVoxel") {
            auto j = getCellPIDs(c); // size: 8
            geo.pushQuad(j[0], j[1], j[3], j[2]);
            geo.pushQuad(j[4], j[5], j[7], j[6]);
            geo.pushQuad(j[0], j[1], j[5], j[4]);
            geo.pushQuad(j[1], j[3], j[7], j[5]);
            geo.pushQuad(j[3], j[2], j[6], j[7]);
            geo.pushQuad(j[2], j[0], j[4], j[6]);
            continue;
        }

        if (type == "vtkTetra") {
            auto j = getCellPIDs(c); // size: 4
            geo.pushTri(j[0], j[2], j[1]);
            geo.pushTri(j[0], j[1], j[3]);
            geo.pushTri(j[1], j[2], j[3]);
            geo.pushTri(j[2], j[0], j[3]);
            continue;
        }

        if (type == "vtkPyramid") {
            auto j = getCellPIDs(c); // size: 5
            geo.pushQuad(j[0], j[3], j[2], j[1]);
            geo.pushTri(j[0], j[1], j[4]);
            geo.pushTri(j[1], j[2], j[4]);
            geo.pushTri(j[2], j[3], j[4]);
            geo.pushTri(j[3], j[0], j[4]);
            continue;
        }

        if (type == "vtkWedge") {
            auto j = getCellPIDs(c); // size: 6
            geo.pushTri(j[0], j[1], j[2]);
            geo.pushTri(j[3], j[5], j[4]);
            geo.pushQuad(j[0], j[3], j[4], j[1]);
            geo.pushQuad(j[1], j[4], j[5], j[2]);
            geo.pushQuad(j[2], j[5], j[3], j[0]);
            continue;
        }

        if (type == "vtkHexahedron") {
            auto j = getCellPIDs(c); // size: 8
            geo.pushQuad(j[0], j[3], j[2], j[1]);
            geo.pushQuad(j[4], j[5], j[6], j[7]);
            geo.pushQuad(j[0], j[1], j[5], j[4]);
            geo.pushQuad(j[1], j[2], j[6], j[5]);
            geo.pushQuad(j[2], j[3], j[7], j[6]);
            geo.pushQuad(j[3], j[0], j[4], j[7]);
            continue;
        }

        cout << " unhandled cell type " << type << endl;
        break;
    }

    struct Channel {
        vtkDataArray* a = 0;
        int c = 0;
        int i = 0;
    };

    auto printArray = [&](int i, vtkDataArray* a, vtkDataSetAttributes* set) {
        int size = a->GetNumberOfTuples();
        int comp = a->GetNumberOfComponents();
        std::cout << "\tarray " << i << " is named " << (set->GetArrayName(i) ? set->GetArrayName(i) : "NULL") << std::endl;
        std::cout << "\t size: " << size << " with " << comp << " components per element" << std::endl;
    };

    auto pushDataToTexCoords = [&](vector<Channel> channels, int tcI, vtkDataSetAttributes* set) {
        size_t N = channels[0].a->GetNumberOfTuples();
        if (tcI > 7) { cout << " Warning! too many arrays.. skipping array.." << endl; return; }
        std::cout << "\t put array data in texcoords properties " << tcI << std::endl;
        for (auto& c : channels) {
            printArray(c.i, c.a, set);
            cout << "  push component " << c.c << endl;
        }
        for (int j=0; j<N; j++) {
            Vec3d v;
            for (int i=0; i<channels.size(); i++) v[i] = channels[i].a->GetComponent(j, channels[i].c);
            geo.pushTexCoord(v, tcI);
        }
    };

    cout << "POINT_DATA:\n";
    vtkPointData* points = dataset->GetPointData();
    if (points) {
        std::cout << " contains point data with " << points->GetNumberOfArrays() << " arrays." << std::endl;
        vector<Channel> channels;
        int k = 1;
        for (int i = 0; i < points->GetNumberOfArrays(); i++) {
            vtkDataArray* a = points->GetArray(i);
            int Nc = a->GetNumberOfComponents();
            for (int j=0; j<Nc; j++) {
                channels.push_back( { a, j, i } );
                if (channels.size() == 3) {
                    pushDataToTexCoords(channels, k, points);
                    k++;
                    channels.clear();
                }
            }
        }
        if (channels.size() > 0) pushDataToTexCoords(channels, k, points);
    }

    cout << "CELL_DATA:\n";
    vtkCellData* cd = dataset->GetCellData();
    if (cd) {
        std::cout << " contains cell data with " << cd->GetNumberOfArrays() << " arrays." << std::endl;
        for (int i = 0; i < cd->GetNumberOfArrays(); i++) {
            vtkDataArray* a = cd->GetArray(i);
            Channel c1 = {a, 0, i};
            pushDataToTexCoords({c1}, i+1, cd);
        }
    }


    cout << "FIELD_DATA:\n";
     if (dataset->GetFieldData()) {
       std::cout << " contains field data with "
            << dataset->GetFieldData()->GetNumberOfArrays()
             << " arrays." << std::endl;
        for (int i = 0; i < dataset->GetFieldData()->GetNumberOfArrays(); i++)
          {
          std::cout << "\tArray " << i
               << " is named " << dataset->GetFieldData()->GetArray(i)->GetName()
               << std::endl;
          }
        }

    string name = "vtk";

    auto m = VRMaterial::create(name + "_mat");
    m->setLit(0);
    m->setDiffuse(Color3f(0.3,0.7,1.0));

    VRGeometryPtr g = geo.asGeometry(name);
    g->setMaterial(m);
    //g->updateNormals();
    res->addChild( g );
}


struct VTKProject {
    string version;
    string title;
    string format;
    string dataset; // STRUCTURED_POINTS STRUCTURED_GRID UNSTRUCTURED_GRID POLYDATA RECTILINEAR_GRID FIELD

    string toString() {
        string res = " VTK project " + title + ", version " + version + ", format " + format;
        if (dataset.size()) res += ", dataset " + dataset;
        return res;
    }
};

void loadVtk_old(string path, VRTransformPtr res) {
    cout << "load VTK file " << path << endl;
    ifstream file(path.c_str());
    string line;

    auto next = [&]() -> string& {
        getline(file, line);
        return line;
    };

    VTKProject project;
    project.version = splitString( next() )[4];
    project.title = next();
    project.format = next();
    project.dataset = splitString( next() )[1];

    VRGeoData geo; // build geometry

    if (project.dataset == "STRUCTURED_POINTS") {
        auto r = splitString( next() ); Vec3i dims = toValue<Vec3i>( r[1] + " " + r[2] + " " + r[3] );
             r = splitString( next() ); Vec3d p0 = toValue<Vec3d>( r[1] + " " + r[2] + " " + r[3] );
             r = splitString( next() ); Vec3d d = toValue<Vec3d>( r[1] + " " + r[2] + " " + r[3] );

        for (int k=0; k<dims[2]; k++) {
            for (int j=0; j<dims[1]; j++) {
                for (int i=0; i<dims[0]; i++) {
                    geo.pushVert(p0 + Vec3d(d[0]*i, d[1]*j, d[2]*k) );
                    geo.pushPoint();
                }
            }
        }
    }

    if (project.dataset == "STRUCTURED_GRID") {
        auto r = splitString( next() ); Vec3i dims = toValue<Vec3i>( r[1] + " " + r[2] + " " + r[3] );
             r = splitString( next() ); int N = toInt(r[1]); string type = r[2]; // points

        vector<Vec3d> points;
        for (int i=0; i<N; i++) {
            Vec3d p = toValue<Vec3d>( next() );
            points.push_back(p);
            geo.pushVert(p);
            geo.pushPoint();
        }
    }

    if (project.dataset == "RECTILINEAR_GRID") {
        ;
    }

    if (project.dataset == "UNSTRUCTURED_GRID") {
        ;
    }

    if (project.dataset == "POLYDATA") {
        auto r = splitString( next() ); int N = toInt(r[1]); string type = r[2]; // points
        for (int i=0; i<N; i++) geo.pushVert( toValue<Vec3d>( next() ) );

        while (next() != "") {
            r = splitString( line );
            string type = r[0];
            N = toInt(r[1]);
            //int size = toInt(r[2]);
            for (int i=0; i<N; i++) { // for each primitive
                r = splitString( next() );
                int Ni = toInt(r[0]); // length of primitive
                cout << line << "  " << Ni << endl;
                //if (Ni == 2) geo.pushLine(toInt(r[1]), toInt(r[2]));
                if (Ni == 3) geo.pushTri(toInt(r[1]), toInt(r[2]), toInt(r[3]));
                if (Ni == 4) geo.pushQuad(toInt(r[1]), toInt(r[2]), toInt(r[3]), toInt(r[4]));
            }
        }
    }

    if (project.dataset == "FIELD") {
        ;
    }

    // parsing finished
    cout << project.toString() << endl;
    file.close();

    auto m = VRMaterial::create(project.title + "_mat");
    m->setLit(0);
    m->setDiffuse(Color3f(0.3,0.7,1.0));

    VRGeometryPtr g = geo.asGeometry(project.title);
    g->setMaterial(m);
    //g->updateNormals();
    res->addChild( g );
}

OSG_END_NAMESPACE;
