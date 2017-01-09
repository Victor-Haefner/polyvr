
#include "rpmTool.h"
#include <string>
#include <iostream>
#include <fstream>

#include "arrayOut.h"

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

double* rpmSpectrum::operator()(const double rpm) {
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
    // upper bound
    double x1 = it->first;
    double* y1 = it->second.ptr();

    std::advance(it, -1);

    // lower bound (check)
    double x0 = it->first;
    double* y0 = it->second.ptr();

    // interpolate current
    for (uint i = 0; i < _resolution; ++i) {
      interpLin(_current[i], rpm, x0, x1, y0[i], y1[i]);
    }
  }
  _last = rpm;
  arrayToFile audioToFile("../rpmSpectrum", _current, _resolution);
  return _current;
}

void rpmSpectrum::interpLin(double& y, const double &x, const double &x0, const double &x1, const double &y0, const double &y1) {
  y =  y0 + (x - x0) * (y1 - y0) / (x1 - x0);
}

double rpmSpectrum::getMin() {
  return _data.begin()->first;
}

double rpmSpectrum::getMax() {
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
    double* labels = new double [_nSamples];
    for (int i=0; i<_nSamples; ++i){
      file >> labels[i];
    }


    // 4th line should be data matrix
    for (int i=0; i<_nSamples; ++i) {

      spectrum tmp(_resolution);

      for (int j=0; j<_resolution; ++j){
        file >> tmp[j];
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

VRMotorPtr VRMotor::create() { return VRMotorPtr( new VRMotor() ); }

void VRMotor::load(const char*filename) {
  if (_tool.isLoaded()) {
    std::cout<<"Spectrum data already loaded"<<std::endl;
    return;
  }
  _tool.readFile(filename);
  _minRPM = _tool.getMin();
  _maxRPM = _tool.getMax();
  std::cout<<"Spectrum tool loaded"<<std::endl;
}

void VRMotor::play(double rpm) {
  if (!_tool.isLoaded()) {
    std::cout<<"No spectrum data loaded"<<std::endl;
    return;
  }
  _sound.synthesizeSpectrum(_tool(rpm), _tool.getRes());
}

void VRMotor::play() {
  if (!_tool.isLoaded()) {
    std::cout<<"No spectrum data loaded"<<std::endl;
    return;
  }
  _sound.synthesizeSpectrum(_tool(_persistent), _tool.getRes());
}

void VRMotor::setRPM(double rpm) {
    if (rpm < _minRPM) _persistent = _minRPM;
    else if (rpm > _maxRPM) _persistent = _maxRPM;
    else _persistent = rpm;
}

} // end namespace rpmTool


