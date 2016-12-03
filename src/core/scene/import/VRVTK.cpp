#include "VRVTK.h"

#include <vtkXMLParser.h>
#include <vtkDataSetReader.h>
#include <vtkDataSet.h>
#include <vtkDataArray.h>
#include <vtkCellData.h>
#include <vtkPointData.h>
#include <vtkCell.h>
#include <vtkQuad.h>
#include <vtkLine.h>

/* NOT COMPILING?
sudo apt-get install libvtk5-dev
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

    /*auto parser = vtkXMLParser::New();
    parser->SetFileName(path.c_str());
    parser->Parse();*/

    vtkDataSetReader* reader = vtkDataSetReader::New();
    reader->SetFileName(path.c_str());
    reader->ReadAllScalarsOn();
    reader->ReadAllVectorsOn();
    reader->ReadAllNormalsOn();
    reader->ReadAllTensorsOn();
    reader->ReadAllTCoordsOn();
    reader->ReadAllFieldsOn();
    reader->ReadAllColorScalarsOn();
    reader->Update();

    vtkDataSet* dataset = reader->GetOutput();

    int npoints = dataset->GetNumberOfPoints();
    int ncells = dataset->GetNumberOfCells();
    int nscalars = reader->GetNumberOfScalarsInFile();
    int nvectors = reader->GetNumberOfVectorsInFile();
    int ntensors = reader->GetNumberOfTensorsInFile();
    cout << "dataset sizes: " << npoints << " " << ncells << " " << nscalars << " " << nvectors << " " << ntensors << endl;

    for (int i=0; i<npoints; i++) {
        auto p = dataset->GetPoint(i);
        Vec3f v(p[0], p[1], p[2]);
        geo.pushVert(v);
        cout << "point " << v << endl;
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
        cout << "cell type " << type << endl;
        if (type == "vtkQuad") {
            auto j = getCellPIDs(c);
            geo.pushQuad(j[0], j[1], j[2], j[3]);
        }
    }

    //vtkCellData* cells = dataset->GetCellData();
    vtkPointData* points = dataset->GetPointData();

    cout << "POINT_DATA:\n";
    if (points) {
        std::cout << " contains point data with " << points->GetNumberOfArrays() << " arrays." << std::endl;
        for (int i = 0; i < points->GetNumberOfArrays(); i++) {
            std::cout << "\tArray " << i << " is named " << (points->GetArrayName(i) ? points->GetArrayName(i) : "NULL") << std::endl;
        }

        for(int i=0; vtkDataArray* a = points->GetArray(i); i++ ) {
            int size = a->GetNumberOfTuples();
            int comp = a->GetNumberOfComponents();
            cout << " data array " << size << " " << comp << endl;

            for (int j=0; j<size; j++) {
                cout << "pnt:";
                for (int k=0; k<comp; k++) cout << " " << a->GetComponent(j, k);
                cout << endl;
            }
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


    cout << "CELL_DATA:\n";

     vtkCellData *cd = dataset->GetCellData();
      if (cd)
        {
        std::cout << " contains cell data with "
             << cd->GetNumberOfArrays()
             << " arrays." << std::endl;
        for (int i = 0; i < cd->GetNumberOfArrays(); i++)
          {
          std::cout << "\tArray " << i
               << " is named "
               << (cd->GetArrayName(i) ? cd->GetArrayName(i) : "NULL")
               << std::endl;
          }
        }

    /*if (cells) {
        for(int i=0; vtkDataArray* a = points->GetArray(i); i++ ) {
            int size = a->GetNumberOfTuples();
            int comp = a->GetNumberOfComponents();
            for (int j=0; j<size; j++) {
                cout << "cell:";
                for (int k=0; k<comp; k++) cout << " " << a->GetComponent(j, k);
                cout << endl;
            }
        }
    }*/

    string name = "vtk";

    auto m = VRMaterial::create(name + "_mat");
    m->setLit(0);
    m->setDiffuse(Vec3f(0.3,0.7,1.0));

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
        auto r = splitString( next() ); Vec3i dims = toVec3i( r[1] + " " + r[2] + " " + r[3] );
             r = splitString( next() ); Vec3f p0 = toVec3f( r[1] + " " + r[2] + " " + r[3] );
             r = splitString( next() ); Vec3f d = toVec3f( r[1] + " " + r[2] + " " + r[3] );

        for (int k=0; k<dims[2]; k++) {
            for (int j=0; j<dims[1]; j++) {
                for (int i=0; i<dims[0]; i++) {
                    geo.pushVert(p0 + Vec3f(d[0]*i, d[1]*j, d[2]*k) );
                    geo.pushPoint();
                }
            }
        }
    }

    if (project.dataset == "STRUCTURED_GRID") {
        auto r = splitString( next() ); Vec3i dims = toVec3i( r[1] + " " + r[2] + " " + r[3] );
             r = splitString( next() ); int N = toInt(r[1]); string type = r[2]; // points

        vector<Vec3f> points;
        for (int i=0; i<N; i++) {
            Vec3f p = toVec3f( next() );
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
        for (int i=0; i<N; i++) geo.pushVert( toVec3f( next() ) );

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
    m->setDiffuse(Vec3f(0.3,0.7,1.0));

    VRGeometryPtr g = geo.asGeometry(project.title);
    g->setMaterial(m);
    //g->updateNormals();
    res->addChild( g );
}

OSG_END_NAMESPACE;
