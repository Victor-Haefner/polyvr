#ifndef GEOIO_H
#define GEOIO_H

#include <OpenSG/OSGSceneFileHandler.h>
#include <OpenSG/OSGGeometry.h>
#include "core/objects/geometry/OSGGeometry.h"

using namespace std;

OSG_BEGIN_NAMESPACE;

/**
* Static utility class for saving && loading OSG- && VRGeometry
**/
class GeoIO
{
    public:

        /**
        * Saves a Geometry in an .osb binary file.
        * @param _frameid: the id of the frame (use for path writing)
        * @param the geometry as GeometryMTRecPtr
        * @param the path to save to
        */
        static void save(uint _frameID, GeometryMTRecPtr _geo, string _path){

            string dataName = getDataName(_frameID, _path, ".osb");

            NodeMTRecPtr ngeo = makeNodeFor(_geo);
            SceneFileHandler::the()->write( ngeo, dataName.c_str() );
            cout<< "Saved in: " << dataName << endl;

        }

        /**
        * Loads a VRGeometry from an .osb binary file && writes it in a given Frame.
        * @param _frame: a pointer to the pointer which has to contain the VrGeometry
        * @param the path to load from
        * @return true if the path was found.
        */
        static bool load(frame* _frame, string _path){

            string dataName = getDataName(_frame->id, _path, ".osb");
            ifstream stream(dataName.c_str());
            bool fileLoaded = false;

            if (stream) {
                cout << "Loading osb..." << endl;
                NodeMTRecPtr geoNode = SceneFileHandler::the()->read(dataName.c_str());
                _frame->geo = VRGeometry::create("myNewMesh");
                _frame->geo->setMesh( OSGGeometry::create( dynamic_cast<Geometry*>(geoNode->getCore()) ) );
                fileLoaded = true;
            }

            stream.close();
            return fileLoaded;
        }

    private:

        /**
        * Help method for writing a data path.
        * @param _frameID: the file's number
        * @param the path to save to
        * @param the extension of the file
        * @return the path as string.
        */
        static string getDataName(uint _frameID, string _path, string _suffixe){
            stringstream ss;
            ss << _frameID;
            string dataName = _path + ss.str() + _suffixe;
            return dataName;
        }

};

OSG_END_NAMESPACE;

#endif // GEOIO_H
