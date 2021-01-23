#include "datarow.h"
#include "core/utils/toString.h"
#include <iostream>

using namespace OSG;

template<> string typeName(const Datarow& p) { return "Datarow"; }

Datarow::Datarow() {}
Datarow::~Datarow() {}

DatarowPtr Datarow::create() { return DatarowPtr( new Datarow() ); }

size_t Datarow::length() { return data.size(); }

void Datarow::setData(vector<double>& d) { data = d; }
vector<double>& Datarow::getData() { return data; }
vector<double> Datarow::getDataCpy() { return data; }

void Datarow::append(double d) { data.push_back(d); }
void Datarow::resize(int N, double v) { data = vector<double>(N, v); }

void Datarow::set(double d, int i) {
    if (i < 0) i += length();
    if (i < 0 || i >= (int)length()) return;
    data[i] = d;
}

double Datarow::get(int i) {
    if (i < 0) i += length();
    if (i < 0 || i >= (int)length()) return 0;
    return data[i];
}

void Datarow::add(DatarowPtr d) {
    size_t dN = d->length();
    int N = min(length(), dN);
    int k1 = length()-N;
    int k2 = dN-N;
    for (int i=0; i<N; i++) data[i+k1] += d->data[i+k2];
}

void Datarow::insert(int i, double v) {
    if (i < 0) i += length();
    if (i < 0 || i >= (int)length()) return;
    data.insert(data.begin() + i, v);
}

vector<double> Datarow::getMinMax() {
    if (length() == 0) return vector<double>();
    vector<double> res = { data[0], data[0] };
    for (auto d : data) {
        if (d < res[0]) res[0] = d;
        if (d > res[1]) res[1] = d;
    }
    return res;
}

double Datarow::getPCT(int i) {
    if (i == 0) return 0;
    if (i < 0) i += length();
    if (data[i-1] == 0) return 0;
    return (data[i] - data[i-1])/data[i-1];
}

DatarowPtr Datarow::getPCTs() {
    auto res = create();
    for (size_t i=1; i<length(); i++) res->append( getPCT(i) );
    return res;
}

double Datarow::getLogRet(int i) {
    if (i == 0) return 0;
    if (i < 0) i += length();
    if (data[i-1] == 0) return 0;
    return log(data[i]/data[i-1]);
}

DatarowPtr Datarow::getLogRets() {
    auto res = create();
    for (size_t i=1; i<length(); i++) res->append( getLogRet(i) );
    return res;
}

double Datarow::computeAverage(int from, int to) {
    if (to == -1) to = data.size();
    double a = 0;
    for (int i=from; i<to; i++) a += data[i];
    return a/(to-from);
}

double Datarow::computeDeviation(double average, int from, int to) {
    if (to == -1) to = data.size();
    double dv = 0;
    for (int i=from; i<to; i++) {
        double d = data[i]-average;
        dv += d*d;
    }
    return sqrt(dv/(to-from-1));
}

double Datarow::computeCovariance(DatarowPtr other, double average1, double average2, int offset1, int offset2) {
    int N = min(data.size()-offset1, other->data.size()-offset2);
    if (N <= 1) return 0;

    double cov = 0;
    for (int i=0; i<N; i++) {
        double d1 = data[offset1+i]-average1; // read from last element
        double d2 = other->data[offset2+i]-average2; // read from last element
        cov += d1*d2;
    }
    return cov / (N-1);
}

double Datarow::computeCorrelation(DatarowPtr other, int offset1, int offset2) {
    int N = min(data.size()-offset1, other->data.size()-offset2);
    if (N <= 1) return 0;

    double avg1 = computeAverage(offset1, offset1+N);
    double avg2 = other->computeAverage(offset2, offset2+N);

    double dev1 = computeDeviation(avg1, offset1, offset1+N);
    if (dev1 == 0) return 0;
    double dev2 = other->computeDeviation(avg2, offset2, offset2+N);
    if (dev2 == 0) return 0;

    double cov = computeCovariance(other, avg1, avg2, offset1, offset2);
    //cout << "Datarow::computeCorrelation avg, dev: " << avg1 << " " << avg2 << " " << dev1 << " " << dev2 << ", N: " << N << ", cov: " << cov << endl;
    return cov/dev1/dev2;
}
