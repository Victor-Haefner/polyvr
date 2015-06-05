#include "VRAMLLoader.h"

#include <iostream>

#include <boost/foreach.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include <OpenSG/OSGSceneFileHandler.h>
#include <OpenSG/OSGGeoProperties.h>
#include <OpenSG/OSGGeometry.h>

#include "core/objects/object/VRObject.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/material/VRMaterial.h"

#include "core/objects/geometry/VRPhysics.h"


#define AI_MAX_NUMBER_OF_TEXTURECOORDS   0x8


using namespace boost;
using namespace boost::property_tree;
OSG_BEGIN_NAMESPACE;

VRObject* root;
string directory;

VRAMLLoader::VRAMLLoader() {
    ;
}


VRAMLLoader* VRAMLLoader::get() {
    static VRAMLLoader* l = new VRAMLLoader();
    return l;
}


struct Geo {
    GeoVectorPropertyRecPtr tc1 = 0;
    GeoVectorPropertyRecPtr tc2 = 0;
    GeoVectorPropertyRecPtr tc3 = 0;
    GeoVectorPropertyRecPtr tc4 = 0;
    GeoVectorPropertyRecPtr tc5 = 0;
    GeoVectorPropertyRecPtr tc6 = 0;
    GeoVectorPropertyRecPtr tc7 = 0;

    GeoVectorPropertyRecPtr pos = 0;
    GeoVectorPropertyRecPtr norms = 0;
    GeoVectorPropertyRecPtr cols = 0;
    GeoIntegralPropertyRefPtr inds_p = 0;
    GeoIntegralPropertyRefPtr inds_n = 0;
    GeoIntegralPropertyRefPtr inds_c = 0;
    GeoIntegralPropertyRefPtr types = 0;
    GeoIntegralPropertyRefPtr lengths = 0;
    VRGeometry* geo = 0;
    VRMaterial* mat = 0;

    Geo(string name) {
        geo = new VRGeometry(name);
        mat = new VRMaterial(name);

        tc1 = GeoPnt3fProperty::create();
        tc2 = GeoPnt3fProperty::create();
        tc3 = GeoPnt3fProperty::create();
        tc4 = GeoPnt3fProperty::create();
        tc5 = GeoPnt3fProperty::create();
        tc6 = GeoPnt3fProperty::create();
        tc7 = GeoPnt3fProperty::create();

        pos = GeoPnt3fProperty::create();
        norms = GeoVec3fProperty::create();
        cols = GeoVec3fProperty::create();
        inds_p = GeoUInt32Property::create();
        inds_n = GeoUInt32Property::create();
        inds_c = GeoUInt32Property::create();
        types = GeoUInt32Property::create();
        lengths = GeoUInt32Property::create();
    }

    void finish() {
        geo->setTypes(types);
        geo->setLengths(lengths);
        geo->setPositions(pos);
        geo->setNormals(norms);
        geo->setColors(cols);
        geo->getMesh()->setIndex(inds_p, Geometry::PositionsIndex);
        geo->getMesh()->setIndex(inds_c, Geometry::ColorsIndex);
        geo->getMesh()->setIndex(inds_n, Geometry::NormalsIndex);
        geo->setTexCoords(tc1, 0);
        geo->setTexCoords(tc2, 1);
        geo->setTexCoords(tc3, 2);
        geo->setTexCoords(tc4, 3);
        geo->setTexCoords(tc5, 4);
        geo->setTexCoords(tc6, 5);
        geo->setTexCoords(tc7, 6);
        geo->setMaterial(mat);
    }
};

void VRAMLLoader::buildMesh(string path, Matrix4f m) {


cout << "Building Mesh" << endl;
    /*Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile( path,
        aiProcess_CalcTangentSpace       |
        aiProcess_Triangulate            |
        aiProcess_JoinIdenticalVertices  |
        aiProcess_SortByPType);

  if( !scene)
  {
    cout<< "Error !" << importer.GetErrorString()<< endl;;

  }

    for( int i =0; i < scene->mNumMeshes; i++)
    {

    aiMesh* mesh = scene->mMeshes[i];

   // cout << mesh->mNumBones << endl;
    Geo geo("text");
    int vertexNumber = 0;
    int facesNumber = 0;
    int currentIndsPos = 0;

   for(int j = 0; j < mesh->mNumVertices; j++)
        {
            float x = mesh->mVertices[j].x;
            float y = mesh->mVertices[j].y;
            float z = mesh->mVertices[j].z;
           // cout << x << " " << y << " " << z << " " << endl;
            geo.pos->addValue(Vec3f(x,y,z));
            vertexNumber += 1;
        }

        //define geo.norms
        float x = mesh->mNormals->x;
        float y = mesh->mNormals->y;
        float z = mesh->mNormals->z;
        geo.norms->addValue(Vec3f(x,y,z));

        //define geo.inds_n and geo.inds_p
        for(int j = 0; j < mesh->mNumFaces; j++)
        {
            const struct aiFace* face = &mesh->mFaces[j];


            for(int k = 0; k < face->mNumIndices; k++)
            {
                geo.inds_p->addValue(face->mIndices[k]);
                geo.inds_n->addValue(i);
            }
        }

        facesNumber += mesh->mNumFaces;

        //create TextCoords method
        for (int i=0; i<vertexNumber; i++) { // per vertex
            //geo.cols->addValue(Vec4f(0,0,1,1));
            geo.tc1->addValue(Vec2f(0,0));
        }

        geo.types->addValue(GL_TRIANGLES);
        geo.lengths->addValue(3*facesNumber);

        aiColor3D diffuse (0.f,0.f,0.f);
        aiColor3D specular (0.f,0.f,0.f);
        aiColor3D ambient (0.f,0.f,0.f);
        aiColor3D emissive (0.f,0.f,0.f);
        float shininess (0.f);

        int matIndex = mesh->mMaterialIndex;
        aiMaterial* mat = scene->mMaterials[matIndex];

        mat->Get(AI_MATKEY_COLOR_DIFFUSE,diffuse);
        mat->Get(AI_MATKEY_COLOR_SPECULAR,specular);
        mat->Get(AI_MATKEY_COLOR_AMBIENT,ambient);
        mat->Get(AI_MATKEY_COLOR_EMISSIVE,emissive);
        mat->Get(AI_MATKEY_SHININESS,shininess);

        geo.mat->setDiffuse(Vec3f(diffuse.r,diffuse.g,diffuse.b));
        geo.mat->setAmbient(Vec3f(ambient.r, ambient.g, ambient.b));
        geo.mat->setSpecular(Vec3f(specular.r, specular.g, specular.b));
        geo.mat->setTransparency(1);
        geo.mat->setShininess(shininess);
        geo.mat->setEmission(Vec3f(emissive.r,emissive.g,emissive.b));

        geo.finish();



        //m.setScale(0.001,0.001,0.001);

        for (int i = 0; i < 4; i++)
        {
               cout << m[i][0] << " "
               << m[i][1] << " "
               << m[i][2] << " "
               << m[i][3] << endl;

        }

        geo.geo->setMatrix(m);
        root->addChild(geo.geo);
    }*/

}
int meshNumber = 0;
void VRAMLLoader::print(boost::property_tree::ptree const& pt, Matrix4f m)
{
    string nextMesh;

    using boost::property_tree::ptree;
    ptree::const_iterator end = pt.end();
    for (ptree::const_iterator it = pt.begin(); it != end; ++it) {

        if(it->first=="matrix")
        {
            string s (it->second.get_value<std::string>());
            string str2(" ");
           string::size_type sz;

            Matrix4f tempMatrix;
           for(int i = 0; i < 4; i++)
           {
               for(int j = 0; j < 4; j++)
               {
                   if(j == 3 && i==2)
                   {
                        size_t number = s.find(str2);
                    if(number <= s.length())
                    {
                        string variable (s.begin(), s.begin() + number);

                        string temp (s.begin()+number+1, s.end());
                        s = temp;
                       tempMatrix[i][j] = stof(variable);
                       //cout << stof(variable) << endl;
                    }
                    else
                    {
                        string variable (s.begin(), s.end());

                        tempMatrix[i][j] = stof(variable);
                        //cout << stof(variable) << endl;
                    }
                   }
                   else
                   {
                    size_t number = s.find(str2);
                    if(number <= s.length())
                    {
                        string variable (s.begin(), s.begin() + number);

                        string temp (s.begin()+number+1, s.end());
                        s = temp;
                       tempMatrix[i][j] = stof(variable);
                       //cout << stof(variable) << endl;
                    }
                    else
                    {
                        string variable (s.begin(), s.end());

                        tempMatrix[i][j] = stof(variable);
                        //cout << stof(variable) << endl;
                    }
                   }

               }
               //cout << m[i][0] << " " << m[i][1] << " " << m[i][2] << " " << m[i][3] << endl;
           }
            Matrix4f temp2;
                    for(int z = 0; z < 4; z++)
                    {
                        for(int k = 0; k < 4; k++)
                        {
                            temp2[z][k] = m[z][0]*tempMatrix[0][k]
                                + m[z][1]*tempMatrix[1][k]
                                + m[z][2]*tempMatrix[2][k]
                                + m[z][3]*tempMatrix[3][k];
                        }
                    }

            m = temp2;

        }

       if(it->first=="url")
        {
            string str(it->second.get_value<std::string>());
            string str2("#");
            size_t found = str.find(str2);
            string s7b (str.begin(), str.begin()+found);
            nextMesh = s7b;
            cout << directory  <<  "/" <<  nextMesh << endl;
            loadProducts(directory + "/" + nextMesh, m);

        }
        else
        {
         print(it->second,m);
        }
    }

}
void VRAMLLoader::loadProducts(string path, Matrix4f m)
{
    /*Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile( path,
        aiProcess_CalcTangentSpace       |
        aiProcess_Triangulate            |
        aiProcess_JoinIdenticalVertices  |
        aiProcess_SortByPType);

  if( !scene)
  {
    cout<< "Error !" << importer.GetErrorString()<< endl;;

  }
    int numberMesh = 0;


    using boost::property_tree::ptree;
    ptree pt;
    read_xml(path, pt);
    bool product = true;

     BOOST_FOREACH( const ptree::value_type &v, pt.get_child("COLLADA")) {
        if(v.first=="library_geometries")
        {
           product = false;
           break;
        }
    }

    if(product)
    {
        print(pt.get_child("COLLADA").get_child("library_nodes").get_child("node"),m);
    }
    else
    {
        m.transpose();
        buildMesh(path,m);
    }*/

}

VRObject* VRAMLLoader::load(string path) {

    root = new VRObject("aml_root");
    /**
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile( path,
        aiProcess_CalcTangentSpace       |
        aiProcess_Triangulate            |
        aiProcess_JoinIdenticalVertices  |
        aiProcess_SortByPType);

  if( !scene)
  {
    cout<< "Error !" << importer.GetErrorString()<< endl;;

  }
    for(int i = 0; i < 4; i++)
    {


    cout << scene->mRootNode->mTransformation[i][0] << " "
    << scene->mRootNode->mTransformation[i][1] << " "
    << scene->mRootNode->mTransformation[i][2] << " "
    << scene->mRootNode->mTransformation[i][3] << endl;
    }

    cout << scene->mRootNode->mNumChildren << endl;
    cout << scene->mRootNode->mName.data << endl;

    cout << scene->mRootNode->mChildren[0]->mName.data << endl;
    cout << scene->mRootNode->mChildren[0]->mNumChildren << endl;

    cout << scene->mRootNode->mChildren[0]->mChildren[0]->mName.data << endl;
    cout << scene->mRootNode->mChildren[0]->mChildren[0]->mNumChildren << endl;

    cout << scene->mRootNode->mChildren[0]->mChildren[0]->mChildren[0]->mName.data << endl;
    cout << scene->mRootNode->mChildren[0]->mChildren[0]->mChildren[0]->mNumChildren << endl;

    for(int j = 0; j < 4; j++)
         {
            cout << scene->mRootNode->mChildren[0]->mChildren[0]->mChildren[0]->mParent->mTransformation[j][0] << " "
            << scene->mRootNode->mChildren[0]->mChildren[0]->mChildren[0]->mParent->mTransformation[j][1]  << " "
            << scene->mRootNode->mChildren[0]->mChildren[0]->mChildren[0]->mParent->mTransformation[j][2]  << " "
            << scene->mRootNode->mChildren[0]->mChildren[0]->mChildren[0]->mParent->mTransformation[j][3]  << " "<< endl;
         }

    for(int j = 0; j < 4; j++)
         {
            cout << scene->mRootNode->mChildren[0]->mChildren[0]->mChildren[0]->mTransformation[j][0] << " "
            << scene->mRootNode->mChildren[0]->mChildren[0]->mChildren[0]->mTransformation[j][1]  << " "
            << scene->mRootNode->mChildren[0]->mChildren[0]->mChildren[0]->mTransformation[j][2]  << " "
            << scene->mRootNode->mChildren[0]->mChildren[0]->mChildren[0]->mTransformation[j][3]  << " "<< endl;
         }


    cout << scene->mMeshes[0]->mNumFaces << endl;
    cout << scene->mMeshes[0]->mFaces[1].mNumIndices << endl;
    cout << scene->mMeshes[0]->mNumAnimMeshes << endl;
    cout << scene->mMeshes[0]->mNumVertices << endl;
    for(int k = 0; k < 4; k++)
    {

        cout << scene->mMeshes[0]->mBones[k]->mName.data << endl;
         cout << scene->mMeshes[0]->mBones[k]->mNumWeights << endl;
         for (int o = 0; o < scene->mMeshes[0]->mBones[k]->mNumWeights; o++)

         for(int j = 0; j < 4; j++)
         {
            cout << scene->mMeshes[0]->mBones[k]->mOffsetMatrix[j][0]  << " "
            << scene->mMeshes[0]->mBones[k]->mOffsetMatrix[j][1]  << " "
            << scene->mMeshes[0]->mBones[k]->mOffsetMatrix[j][2]  << " "
            << scene->mMeshes[0]->mBones[k]->mOffsetMatrix[j][3]  << " "<< endl;
         }
    }
    **/
    const size_t last_slash_idx = path.rfind('/');

    if (std::string::npos != last_slash_idx)
    {
        directory = path.substr(0, last_slash_idx);
    }
    //cout << directory << endl;
    using boost::property_tree::ptree;
    ptree pt;
    read_xml(path, pt);

    /**
    BOOST_FOREACH( const ptree::value_type &v, pt.get_child("COLLADA")) {
        if(v.first=="instance_node")
        {
            cout<< "Tame sme" << endl;

        }

    }

    **/
           // buildMesh(path);
            //buildMesh(directory + '/');
           //print(pt.get_child("COLLADA").get_child("library_nodes"));
           Matrix4f m {

            0.001f,0,0,0,
            0,0.001f,0,0,
            0,0,0.001f,0,
            0,0,0,0.001f

        };
            loadProducts(path,m);






    /**
    // ------------ PHYSICS -----------------
    auto phys = geo.geo->getPhysics();
    phys->setDynamic(true);
    phys->setShape("Convex");
    phys->setPhysicalized(true);
    **/


    return root;

}
OSG_END_NAMESPACE;
