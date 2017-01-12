#ifndef RPMTOOL_H_INCLUDED
#define RPMTOOL_H_INCLUDED

#include <map>
#include "core/scene/sound/VRSound.h"
#include "VRrpmToolFwd.h"

namespace rpmTool {

class spectrum {
public:
  spectrum(unsigned int length);
  spectrum(const spectrum &other);
  spectrum(){};
  ~spectrum() {
    delete[] _data;
  };
  double* ptr() {return _data; }; // const? does this return a new pointer?
  const unsigned int length() {return _length; };
  double& operator[](const unsigned int i) {return *(_data + i); };

  void print();
private:
  double* _data;
  unsigned int _length;
};

class rpmSpectrum {
public:
  rpmSpectrum();
  ~rpmSpectrum() {
    delete[] _current;
  };
  void print();
  double* operator()(const double rpm);
  void readFile(const char* filename);
  const uint getRes() {return _resolution; };
  const bool isLoaded() {return init; };
  double getMinRPM();
  double getMaxRPM();
  double getMaxSample() {return _maxVal; };
private:
  void interpLin(double& y, const double& x, const double& x0, const double& x1, const double& y0, const double& y1);

  unsigned int _nSamples; // number of spectra provided
  unsigned int _resolution; // number of frequencies per spectrum, data type needs to fit ~50k
  double _last; // previous rpm
  bool init = false;
  double _maxVal = 0.;

  // map: key is rpm, value is spectrum data
  std::map<double, spectrum > _data;

  // current interpolated vector
  double* _current;
};

class VRMotor {
public:
  VRMotor() {};
  static VRMotorPtr create();
  void load(const char* filename);
  void play(double rpm);
  void play();
  void setRPM(double rpm);
  double getRPM() {return _persistent; };
private:
  OSG::VRSound _sound;
  rpmSpectrum _tool;
  double _persistent; // for python testing
  double _minRPM;
  double _maxRPM;
  double _maxVal;
};

} // end namespace rpmTool

#endif
