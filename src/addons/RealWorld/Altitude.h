#ifndef ALTITUDE_H
#define	ALTITUDE_H

#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <OpenSG/OSGVector.h>

using namespace OSG;
using namespace std;

namespace realworld {
/** Works, but probably returns not the right altitudes **/
    class Altitude {
    public:
        // SIZE 1201 || 3601
        static const int SRTM_SIZE = 1201;
        short height[SRTM_SIZE][SRTM_SIZE];
        int cLat;
        int cLng;

       Altitude(){
            //std::copy(height, height + sizeof(height)/sizeof(height[0]), 0);
            for (int i=0; i<SRTM_SIZE*SRTM_SIZE; i++) *(height[0]+i) = 0;
            cLat = 0;
            cLng = 0;
        }

        /*void showHeightArray(float lat, float lng){
            getAltitude(lat, lng);
            for(int i = 0; i < 1200 ; i++){
                for(int j = 0; j < 1200; j++){
                    cout << "[" << height[i][j] << "] ";
                }
                cout << endl;
            }
        }*/

        int getAltitude(float lat, float lng)
        {
            //fill Array with new heights, if it does not contain input position
            if(!(cLat == (int)lat && cLng == (int)lng)){
                cLat = (int)lat;
                cLng = (int)lng;
                fillHeightArray();
                cout << "Hight Array filled!" << endl;
            } else
                cout << "Hight Array exists already." << endl;

            int h =  height[getRow(lat)][getRow(lng)];
            return h;
        }

        void fillHeightArray(){
            string hgtFile = "world/altitudes/" + getHGTFileName(cLat, cLng);
            std::ifstream file(hgtFile.c_str(), std::ios::in|std::ios::binary);
            if(!file)
            {
                std::cout << "Error opening file!" << std::endl;
                return;
            }

            unsigned char buffer[2];
            for (int i = 0; i < SRTM_SIZE; ++i)
            {
                for (int j = 0; j < SRTM_SIZE; ++j)
                {
                    if(!file.read( reinterpret_cast<char*>(buffer), sizeof(buffer) ))
                    {
                        std::cout << "Error reading file!" << std::endl;
                        return;
                    }
                    height[i][j] = (buffer[0] << 8) | buffer[1];
                }
            }
            if(file.is_open()) file.close();
        }

        //get Row || Col
        int getRow(float l){
            //cout << "lon/lat: " << l << endl;
            int temp = (int)((l - (int)l) * 5000000);
            //cout << "lat/long after komma: " << temp << endl;
            double step = (double)SRTM_SIZE / 5000000;
            //cout << "stepsize: " << step << endl;
            int result  = (int)(temp * step);
            cout << "row/col: " <<  result << endl;
            return result;
        }

        //get Name of the file that contains the hight data to the wanted position
        string getHGTFileName(float lat, float lng){
            string north;
            if(lat < 10) north = "N0" + getString((int)lat);
            else north = "N" + getString((int)lat);
            string east;
            if(lng < 10) east = "E00" + getString((int)lng);
            else if(lng < 100) east = "E0" + getString((int)lng);
            else east = "E" + getString((int)lng);
            return north + east + ".hgt";
        }
        string getString(int a){
            stringstream ss;
            ss << a;
            return ss.str();
        }
    };
};

#endif	/* ALTITUDE_H */
