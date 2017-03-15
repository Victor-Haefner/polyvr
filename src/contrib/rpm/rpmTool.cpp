
#include "rpmTool.h"
#include <string>
#include <iostream>
#include <fstream>

#include "arrayOut.h" // TESTING

//#define SPECTRUM_OUTPUT
#define FILTER_SPECTRUM

namespace rpmTool {

spectrum::spectrum(unsigned int length) {
  _length = length;
  _data = new double[_length];
}

spectrum::spectrum(const spectrum& other) : _length(other._length) {
  _data = new double[_length];
  std::copy(other._data, other._data + _length, _data);
}

// change to ostream friend
void spectrum::print() {
  for (int i = 0; i < _length; ++i) {
    std::cout<<*(_data + i)<<" ";
  }
}

rpmSpectrum::rpmSpectrum() {
}

double* rpmSpectrum::operator()(const float rpm) {
  if (rpm == _last) { // could this return garbage: _current == null => _last == null or NaN
    return _current;
  }
  // if we ask for a value that we have exact results for, return those
  auto it = _data.lower_bound(rpm);
  if (it == _data.end()) { // this could return garbage on first call
    std::cout<<"Spectrum tool: requested value out of bounds."<<std::endl;
    return _current; // throw error or clamp range and return?
  }
  if (rpm == it->first) {
    _current = it->second.ptr();
  } else {
    if (_lin) {
      // upper bound
      float x1 = it->first;
      double* y1 = it->second.ptr();

      std::advance(it, -1);

      // lower bound (check)
      float x0 = it->first;
      double* y0 = it->second.ptr();

      // interpolate current
      for (uint i = 0; i < _resolution; ++i) {
        interpLin(_current[i], rpm, x0, x1, y0[i], y1[i]);
      }
  } else { // check if iterator will go to far
      float x2 = it->first;
      double* y2 = it->second.ptr();
      std::advance(it, -1);
      float x1 = it->first;
      double* y1 = it->second.ptr();
      if (abs(rpm -  x1) < abs(rpm -  x2)) {
        std::advance(it, -1);
        float x0 = it->first;
        double* y0 = it->second.ptr();
        for (uint i = 0; i < _resolution; ++i) {
          interpLag3(_current[i], rpm, x0, x1, x2, y0[i], y1[i], y2[i]);
        }
      } else {
        std::advance(it, 2);
        float x3 = it->first;
        double* y3 = it->second.ptr();
        for (uint i = 0; i < _resolution; ++i) {
          interpLag3(_current[i], rpm, x1, x2, x3, y1[i], y2[i], y3[i]);
        }
      }
  }
  }
  _last = rpm;
#ifdef SPECTRUM_OUTPUT
  arrayToFile audioToFile("../spectrumTestData/validationSpectrum1855", _current, _resolution); // TESTING
#endif
  return _current;
}

void rpmSpectrum::interpLin(double& y, const float &x, const float &x0, const float &x1, const double &y0, const double &y1) {
  y =  y0 + (x - x0) * (y1 - y0) / (x1 - x0);
}

void rpmSpectrum::interpLag3(double& y, const float &x, const float &x0, const float &x1, const float &x2, const double &y0, const double &y1, const double &y2) {
  y =  (y0 * (x - x1) * (x - x2)) / ((x0 - x1)*(x0 - x2)) + (y1 * (x - x0) * (x - x2)) / ((x1 - x0)*(x1 - x2)) + (y2 * (x - x0) * (x - x1)) / ((x2 - x0)*(x2 - x1));
}

float rpmSpectrum::getMinRPM() {
  return _data.begin()->first;
}

float rpmSpectrum::getMaxRPM() {
  return _data.rbegin()->first;
}

void rpmSpectrum::readFile(const char* filename) {

  std::ifstream file(filename);

  if (file) {

    std::cout<<"Reading data from file."<<std::endl;
    // check lines or file format?
    file >> _nSamples; // should be 1st line
    std::cout<<"Data:\nSamples: "<<_nSamples<<std::endl;

    file >> _resolution; // should be 2nd line
    std::cout<<"Resolution: "<<_resolution<<std::endl;

    // 3rd line should be label vector
    float* labels = new float [_nSamples];
    for (int i=0; i<_nSamples; ++i){
      file >> labels[i];
    }


    // 4th line should be data matrix
    for (int i=0; i<_nSamples; ++i) {

      spectrum tmp(_resolution);

      for (int j=0; j<_resolution; ++j){
        file >> tmp[j];
        if (abs(tmp[j]) > _maxVal) _maxVal = tmp[j];
      }
      //std::cout<<"Map entry: "<<i<<" Label: "<<labels[i]<<" Spectrum: ";
      //tmp.print();
      //std::cout<<std::endl;

      //_data[ labels[i] ] = tmp;
      _data.insert(std::make_pair(labels[i], tmp));

      //std::cout<<"Entry added. Size: "<<_data.size()<<std::endl;
    }

    delete labels;

  } else {
    std::cout<<"Could not read data from file."<<std::endl;
    return;
  }

  file.close();
  std::cout<<"Data read from file."<<std::endl;
   _current = new double[ _resolution ];
  //for (int i=0;i<_resolution;++i) _current[i] = 0.;
  init = true;
}

void rpmSpectrum::print() {
  std::cout<<"Spectrum data:\nNumber of samples: "<<_nSamples
  <<"\nResolution: "<<_resolution<<std::endl;
  std::cout<<_data.size()<<" labels. Values:"<<std::endl;

  for(auto it=_data.begin(); it!=_data.end();++it) {
    std::cout<<"rpm: "<< it->first<<" Spectrum: "<<std::flush;

    it->second.print();
    std::cout<<std::endl;
  }
}
/*
rpmFilter::rpmFilter() {
}

void rpmFilter::readFile(const char* filename){
std::ifstream file(filename);

  if (file) {

    std::cout<<"Reading data from file."<<std::endl;
    // check lines or file format?
    file >> _nSamples; // should be 1st line
    std::cout<<"Data:\nSamples: "<<_nSamples<<std::endl;

    file >> _resolution; // should be 2nd line
    std::cout<<"Resolution: "<<_resolution<<std::endl;


    // 3rd line should be labels
    for (int i=0; i<_nSamples; ++i){
      vector<Vec2> points
      for (int j=0; j<_resolution) {
        file >>
      }
      for (int j=0; j<_resolution) {
        file >>
      }
      for (int j=0; j<_resolution) {
        file >>
      }
    }

    }
  } else {
    std::cout<<"Could not read data from file."<<std::endl;
    return;
  }

  file.close();
  std::cout<<"Data read from file."<<std::endl;
  init = true;
}
*/
VRMotorPtr VRMotor::create() { return VRMotorPtr( new VRMotor() ); }

void VRMotor::load(const char*filename) {
  if (_tool.isLoaded()) {
    std::cout<<"Spectrum data already loaded"<<std::endl;
    return;
  }
  _tool.readFile(filename);
  _minRPM = _tool.getMinRPM();
  _maxRPM = _tool.getMaxRPM();
  _maxVal = _tool.getMaxSample();
  std::cout<<"Max sample (double): "<<_maxVal<<std::endl;
  std::cout<<"Spectrum tool loaded"<<std::endl;
}

void VRMotor::play(float rpm, float duration, float fade) {
  if (!_tool.isLoaded()) {
    std::cout<<"No spectrum data loaded"<<std::endl;
    return;
  }
  _sound.synthesizeSpectrum(_tool(rpm), _tool.getRes(), duration, fade);
}

void VRMotor::play(float duration, float fade) {
  if (!_tool.isLoaded()) {
    std::cout<<"No spectrum data loaded"<<std::endl;
    return;
  }
  _sound.synthesizeSpectrum(_tool(_persistent), _tool.getRes(), duration, fade);
}

void VRMotor::setRPM(float rpm) {
    if (rpm < _minRPM) _persistent = _minRPM;
    else if (rpm > _maxRPM) _persistent = _maxRPM;
    else _persistent = rpm;
}

} // end namespace rpmTool


