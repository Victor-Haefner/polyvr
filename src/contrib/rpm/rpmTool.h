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
  double* operator()(const float rpm);
  void readFile(const char* filename);
  const uint getRes() {return _resolution; };
  const bool isLoaded() {return init; };
  float getMinRPM();
  float getMaxRPM();
  double getMaxSample() {return _maxVal; };
private:
  bool _lin = true;
  void interpLin(double& y, const float& x, const float& x0, const float& x1, const double& y0, const double& y1);
  void interpLag3(double& y, const float &x, const float &x0, const float &x1, const float &x2, const double &y0, const double &y1, const double &y2);
  unsigned int _nSamples; // number of spectra provided
  unsigned int _resolution; // number of frequencies per spectrum, data type needs to fit ~50k
  float _last; // previous rpm
  bool init = false;
  double _maxVal = 0.;

  // map: key is rpm, value is spectrum data
  std::map<float, spectrum > _data;

  // current interpolated vector
  double* _current;
};

/*
class rpmFilter {
public:
  rpmFilter();
  void readFile(const char* filename);
private:
  std::map<float, vector<Vec2> > _data;
};
*/

class VRMotor {
public:
  VRMotor() {};
  static VRMotorPtr create();
  void load(const char* filename);
  void play(float rpm, float duration, float fade);
  void play(float duration, float fade);
  void setRPM(float rpm);
  float getRPM() {return _persistent; };
  int getQueuedBuffer() { return _sound.getQueuedBuffer(); };
  void recycleBuffer() {_sound.recycleBuffer(); };
private:
  OSG::VRSound _sound;
  rpmSpectrum _tool;
  float _persistent; // for python testing
  float _minRPM;
  float _maxRPM;
  double _maxVal;
};

} // end namespace rpmTool

#endif
