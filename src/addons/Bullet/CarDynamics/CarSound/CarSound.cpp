
#include "CarSound.h"
#include <cmath>
#include <string>
#include <iostream>
#include <fstream>
#include "core/utils/toString.h"

#define FILTER_SPECTRUM

using namespace OSG;

VRCarSound::VRCarSound() {
    sound = VRSound::create();
}

VRCarSound::~VRCarSound() {}
VRCarSoundPtr VRCarSound::create() { return VRCarSoundPtr( new VRCarSound() ); }

const vector<double>& VRCarSound::getSpectrum(const float rpm) {
  if (rpm == lastRPM) return current; // could this return garbage: current == null => last == null or NaN

  // if we ask for a value that we have exact results for, return those
  auto it = data.lower_bound(rpm);
  if (it == data.end()) { // this could return garbage on first call
    cout<<"Spectrum tool: requested value out of bounds."<<endl;
    return current; // throw error or clamp range and return?
  }
  if (rpm == it->first) current = it->second;
  else if (lin) {
      // upper bound
      float x1 = it->first;
      double* y1 = &it->second[0];

      advance(it, -1);

      // lower bound (check)
      float x0 = it->first;
      double* y0 = &it->second[0];

      // interpolate current
      for (uint i = 0; i < resolution; ++i) {
        interpLin(current[i], rpm, x0, x1, y0[i], y1[i]);
      }
  } else { // check if iterator will go to far
      float x2 = it->first;
      double* y2 = &it->second[0];
      advance(it, -1);
      float x1 = it->first;
      double* y1 = &it->second[0];
      if (abs(rpm -  x1) < abs(rpm -  x2)) {
        advance(it, -1);
        float x0 = it->first;
        double* y0 = &it->second[0];
        for (uint i = 0; i < resolution; ++i) {
          interpLag3(current[i], rpm, x0, x1, x2, y0[i], y1[i], y2[i]);
        }
      } else {
        advance(it, 2);
        float x3 = it->first;
        double* y3 = &it->second[0];
        for (uint i = 0; i < resolution; ++i) {
          interpLag3(current[i], rpm, x1, x2, x3, y1[i], y2[i], y3[i]);
        }
      }
  }

  lastRPM = rpm;

  return current;
}

void VRCarSound::interpLin(double& y, const float &x, const float &x0, const float &x1, const double &y0, const double &y1) {
  y =  y0 + (x - x0) * (y1 - y0) / (x1 - x0);
}

void VRCarSound::interpLag3(double& y, const float &x, const float &x0, const float &x1, const float &x2, const double &y0, const double &y1, const double &y2) {
  y =  (y0 * (x - x1) * (x - x2)) / ((x0 - x1)*(x0 - x2)) + (y1 * (x - x0) * (x - x2)) / ((x1 - x0)*(x1 - x2)) + (y2 * (x - x0) * (x - x1)) / ((x2 - x0)*(x2 - x1));
}

const float VRCarSound::getMinRPM() {
  return data.begin()->first;
}

const float VRCarSound::getMaxRPM() {
  return data.rbegin()->first;
}

void VRCarSound::readFile(string filename) {

  ifstream file(filename);

  if (file) {

    cout<<"Reading data from file."<<endl;
    // check lines or file format?
    file >> nSamples; // should be 1st line
    cout<<"Data:\nSamples: "<<nSamples<<endl;

    file >> resolution; // should be 2nd line
    cout<<"Resolution: "<<resolution<<endl;

    // 3rd line should be label vector
    float* labels = new float [nSamples];
    for (uint i=0; i<nSamples; ++i){
      file >> labels[i];
    }


    // 4th line should be data matrix
    for (uint i=0; i<nSamples; ++i) {

      vector<double> tmp(resolution);

      for (uint j=0; j<resolution; ++j){
        file >> tmp[j];
        if (abs(tmp[j]) > maxVal) maxVal = tmp[j];
      }
      //cout<<"Map entry: "<<i<<" Label: "<<labels[i]<<" Spectrum: ";
      //tmp.print();
      //cout<<endl;

      //data[ labels[i] ] = tmp;
      data.insert(make_pair(labels[i], tmp));

      //cout<<"Entry added. Size: "<<data.size()<<endl;
    }

    delete labels;

  } else {
    cout<<"Could not read data from file."<<endl;
    return;
  }

  file.close();
  cout<<"Data read from file."<<endl;
  current = vector<double>(resolution);
  init = true;
}

void VRCarSound::print() {
    cout << "Spectrum data:\nNumber of samples: " <<nSamples <<"\nResolution: "<<resolution<<endl;
    cout << data.size()<<" labels. Values:"<<endl;

    for (auto& it : data) {
        cout << "rpm: " << it.first << " Spectrum: " << flush;
        for (auto d : it.second) cout << d << " ";
        cout << endl;
    }
}

void VRCarSound::resetValues() {
    minRPM = 0.;
    maxRPM = 0.;
    maxVal = 0.;
    nSamples = 0;
    resolution = 0;
    lastRPM = 0.;
    init = false;
}

void VRCarSound::loadSoundFile(string filename) {
//    if (isLoaded()) {
//        cout<<"Spectrum data already loaded"<<endl;
//        return;
//    }
    resetValues();

    readFile(filename);
    minRPM = getMinRPM();
    maxRPM = getMaxRPM();
    cout<<"Max sample (double): "<<maxVal<<endl;
    cout<<"Spectrum tool loaded"<<endl;
}

void VRCarSound::play(float rpm) {
    if (active && sound) {
        if (!isLoaded()) { cout << "No spectrum data loaded" << endl; return; }
        auto clamp = [](float& v, float a, float b) { return min(b,max(a,v)); };
        clamp(rpm, minRPM, maxRPM);

        sound->recycleBuffer();
        if ( sound->getQueuedBuffer() < 2 ) {
            sound->synthSpectrum(getSpectrum(rpm), getRes(), duration, fade);
        }
    }
}

void VRCarSound::toggleSound(bool onOff) { active = onOff; }
void VRCarSound::setSound(VRSoundPtr s) { sound = s; }
VRSoundPtr VRCarSound::getSound() { return sound; }
const uint VRCarSound::getRes() {return resolution; }
const bool VRCarSound::isLoaded() {return init; }
const double VRCarSound::getMaxSample() {return maxVal; }
void VRCarSound::setFade(float f) { fade = f; }
void VRCarSound::setDuration(float d) { duration = d; }



