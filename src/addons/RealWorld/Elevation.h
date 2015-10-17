#ifndef ELEVATION_H
#define	ELEVATION_H

#include <iostream>
#include <fstream>
#include <map>

#include <jsoncpp/json/json.h>
#include <curl/curl.h>
#include <sys/stat.h>

#include <OpenSG/OSGVector.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

class Elevation {
    private:
        Json::Value event;
        const int n = 10;
        const float stepSize = 0.0011;
        const int precision = 2;
        std::map<string, Json::Value> elevations;
        std::map<Vec2f, float> exactElevations;

    public:
        Elevation();

        /** returns elevation for input position && manages near positions, so later access is faster **/
        int getSomeElevation(float lat, float lon);

        /** interpolate surrounding elevations **/
        float getElevation(float lat, float lon);

        /** not finished => to do **/
        void readElevationFile(string id);

        /** create json-file with latitude, longitude && elevation data **/
        void createElevationFile(string id, string latlons[]);

        Json::Value interpolateElevationJson(Json::Value rb);

        /** get String with Json Data for lat- lon pair && their elevation && distance **/
        string getMapQuestElevations(string latlons);

        /** return Json that only contains lat, lon && elevation data **/
        Json::Value getBetterJson(string readBuffer[], Json::Value event);

        string getLatLonName(float lat, float lon);

        bool fileExists(const std::string& filename);

        static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp);

        /** trim from end **/
        string trim(string s);

        /** round float numbers to multiple of 0.0001 **/
        float roundFloat(float f);

        float cutFloat(float f, int p);

        /** converts string to float **/
        float toFloat(string s);
        char* toCharArr(float f);
        string toFloatString(float f, int precision);
};

OSG_END_NAMESPACE;

#endif	/* ELEVATION_H */
