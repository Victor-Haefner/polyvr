#ifndef ELEVATION_H
#define	ELEVATION_H

#include <iostream>
#include <fstream>
#include <map>

//#include "../json/json.h"

#include <jsoncpp/json/json.h>
#include <curl/curl.h>
#include <sys/stat.h>

using namespace OSG;
using namespace std;

namespace realworld {

    class Elevation {
    private:
        Json::Value event;
        const int n;
        const float stepSize;
        const int precision;
        std::map<string, Json::Value> elevations;
        std::map<Vec2f, float> exactElevations;
    public:
        Elevation() : n(10), stepSize(0.001), precision(2){

        }



        /** returns elevation for input position && manages near positions, so later access is faster **/
        int getSomeElevation(float lat, float lon){
            lat = roundFloat(lat);
            lon = roundFloat(lon);
            string latLonName =  toFloatString(lat, 2) + "_" + toFloatString(lon, 2);
            if ( !(elevations.find(latLonName) == elevations.end()) ) { // check if elvation already in array
                Json::Value val = elevations[latLonName];
                return val[toFloatString(lat, 3)][toFloatString(lon, 3)].asInt();
            } //else cout << latLonName << " not found in map!" << endl;

            lat = cutFloat(lat, 2);
            lon = cutFloat(lon, 2);
            string latlons;
            string latlonList[n];
            for(int i = 0; i< n ; i++){
                latlons = "";
                for(int j = 0; j< n; j++){
                    latlons += toFloatString(lat + i*stepSize, 3) + "," + toFloatString(lon+j*stepSize, 3) + ",";
                }
                //cout << "latlons: " << latlons << endl;
                latlonList[i] = latlons;
            }
            string filePath = "world/elevation/" + latLonName + ".json";
            if(fileExists(filePath)){
                //cout << "File exists already!" << endl;
            }else{
                createElevationFile(latLonName, latlonList);
            }
            readElevationFile(latLonName);
            return elevations[latLonName][toFloatString(lat, 3)][toFloatString(lon, 3)].asInt();
        }

        /** interpolate surrounding elevations **/
        float getElevation(float lat, float lon){
            //string fastKey = toFloatString(lat, 6) + "_" + toFloatString(lat, 6);
            if ( !(exactElevations.find(Vec2f(lat, lon)) == exactElevations.end()) ){
                return exactElevations[Vec2f(lat, lon)];
            }
            float prec = 0.001;
            float roundedLat = cutFloat(lat, 3);
            float roundedLon = cutFloat(lon, 3);
            int ele1 = getSomeElevation(roundedLat, roundedLon);
            int ele2 = getSomeElevation(roundedLat + prec, roundedLon);
            int ele3 = getSomeElevation(roundedLat + prec, roundedLon + prec);
            int ele4 = getSomeElevation(roundedLat, roundedLon + prec);
            float deltaLat = lat - roundedLat;
            float deltaLon = lon - roundedLon;
            float eleDeltaLat1 = ele2 * (deltaLat/prec) + ele1 * (1-(deltaLat/prec));
            float eleDeltaLat2 = ele3 * (deltaLat/prec) + ele4 * (1-(deltaLat/prec));
            float eleFinal = eleDeltaLat2 * (deltaLon/prec) + eleDeltaLat1 * (1-(deltaLon/prec));
            eleFinal /= 3.2808399f; //feet to meters
            exactElevations[Vec2f(lat, lon)] = eleFinal;
            return eleFinal;
        }

        /** not finished => to do **/
        void readElevationFile(string id){
            Json::Value root;   // will contains the root value after parsing.
            Json::Reader reader;
            std::ifstream test((char*)("world/elevation/" + id + ".json").c_str(), std::ifstream::binary);
            bool parsingSuccessful = reader.parse( test, root, false );
            if ( !parsingSuccessful )
            {
                // report to the user the failure && their locations in the document.
                std::cout  << reader.getFormatedErrorMessages() << "\n";
                return;
            }else{
                elevations[id] = root;
            }

        }

        /** create json-file with latitude, longitude && elevation data **/
        void createElevationFile(string id, string latlons[]){
                string filePath = "world/elevation/" + id + ".json";
                for(int i = 0; i< n; i++){
                    latlons[i] = getMapQuestElevations(latlons[i]);
                    if(latlons[i] == "error") {
                        cout << "Could not get Elevations from MapQuest!" << endl;
                        return;
                    }
                }

                Json::Value rb;
                rb = getBetterJson(latlons, rb);

                ofstream file;
                file.open((char*)filePath.c_str());
                file << rb;
                file.close();
                cout << "File created!" << endl;

        }

        Json::Value interpolateElevationJson(Json::Value rb){
            return Json::Value();
        }

        /** get String with Json Data for lat- lon pair && their elevation && distance **/
        string getMapQuestElevations(string latlons){
                CURL *curl;
                CURLcode res;
                string readBuffer;

                curl = curl_easy_init();
                if(curl) {
                    string mapquestAppKey = "Fmjtd%7Cluur21urnh%2C2x%3Do5-90ta94";
                    string apiURL = "http://open.mapquestapi.com/elevation/v1/profile?key=" + mapquestAppKey + "&callback=handleHelloWorldResponse&shapeFormat=raw&latLngCollection=" + latlons;
                    curl_easy_setopt(curl, CURLOPT_URL, (char*)apiURL.c_str());
                    /* example.com is redirected, so we tell libcurl to follow redirection*/
                    //curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
                    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
                    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

                    /* Perform the request, res will get the return code*/
                    res = curl_easy_perform(curl);

                    /* always cleanup*/
                    curl_easy_cleanup(curl);

                    /* Check for errors*/
                    if(res != CURLE_OK){
                        return "error";
                    }else{
                        return readBuffer;
                    }
                }

                return "";
        }

        /** return Json that only contains lat, lon && elevation data **/
        Json::Value getBetterJson(string readBuffer[], Json::Value event){
            string rb, sLat, sLon;
            for(int i = 0; i< n; i++){
                rb = readBuffer[i];
                //transform in format for jsoncpp
                rb.erase(0,25); //delete first string + (
                rb.erase(rb.end()-1); //delete ;
                rb.erase(rb.end()-1); //delete )

                Json::Value root;
                Json::Reader reader;
                bool parsingSuccessful = reader.parse(rb, root, false );
                if ( !parsingSuccessful )
                {
                    // report to the user the failure && their locations in the document.
                    cout  << reader.getFormatedErrorMessages() << "\n";
                    return "";
                }

                Json::Value points = root["shapePoints"];
                sLat = toFloatString(points[0].asFloat(), 3);
                for(uint j = 0; j < points.size(); j+=2){
                    sLon = toFloatString(points[j+1].asFloat(), 3);
                    event[(char*)sLat.c_str()][(char*)sLon.c_str()] = root["elevationProfile"][j/2]["height"];
                    //elevationList[sLat + "_" + sLon] = root["elevationProfile"][j/2]["height"].asInt();
                    //cout << "slatslon: " << sLat + "_" + sLon << endl;
                }

            }
            return event;
        }

        string getLatLonName(float lat, float lon){
            return toFloatString(lat, precision) + "_" + toFloatString(lon, precision);
        }


        /**
            Check if a file exists
        @param[in] filename - the name of the file to check

        @return    true if the file exists, else false

        */
        bool fileExists(const std::string& filename)
        {
            struct stat buf;
            if (stat(filename.c_str(), &buf) != -1)
            {
                return true;
            }
            return false;
        }

        static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
        {
            ((std::string*)userp)->append((char*)contents, size * nmemb);
            return size * nmemb;
        }

        /** trim from end **/
        inline string trim(string s) {
                s.erase(s.find_last_not_of(" \n\r\t")+1);
                return s;
        }

        /** round float numbers to multiple of 0.0001 **/
        float roundFloat(float f){
            f += 0.00005; //for exact rounding
            f = (float)(int)(f * 10000);
            return f/10000;
        }

        float cutFloat(float f, int p){
            stringstream ss;
            ss << toFloatString(f, p);
            ss >> f;
            return f;
        }

        /** converts string to float **/
        float toFloat(string s){
            stringstream sstr;
            float f;
            sstr << s;
            sstr >> f;
            return f;
        }
        char* toCharArr(float f){
            std::stringstream os;
            os << f;
            return const_cast<char*>(os.str().c_str());
        }
        string toFloatString(float f, int precision){
            std::stringstream ss;
            ss << std::scientific << std::fixed << std::setprecision(5);
            ss << f;
            string res = ss.str();
            res.erase(res.end()-5+precision, res.end());
            return res;
        }

    };
};

#endif	/* ELEVATION_H */
