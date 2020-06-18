#include "datarow.h"
#include "core/utils/toString.h"

using namespace OSG;

template<> string typeName(const Datarow& p) { return "Datarow"; }

Datarow::Datarow() {}
Datarow::~Datarow() {}

DatarowPtr Datarow::create() { return DatarowPtr( new Datarow() ); }

size_t Datarow::length() { return data.size(); }

void Datarow::setData(vector<double>& d) { data = d; }
vector<double>& Datarow::getData() { return data; }

void Datarow::append(double d) { data.push_back(d); }
void Datarow::resize(int N, double v) { data = vector<double>(N, v); }

void Datarow::set(double d, int i) {
    if (i < 0) i += length();
    if (i < 0 || i >= length()) return;
    data[i] = d;
}

double Datarow::get(int i) {
    if (i < 0) i += length();
    if (i < 0 || i >= length()) return 0;
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
    if (i < 0 || i >= length()) return;
    data.insert(data.begin() + i, v);
}

double Datarow::getPCT(int i) {
    if (i == 0) return 0;
    if (i < 0) i += length();
    if (data[i-1] == 0) return 0;
    return (data[i] - data[i-1])/data[i-1];
}

DatarowPtr Datarow::getPCTs() {
    auto res = create();
    for (int i=1; i<length(); i++) res->append( getPCT(i) );
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
    for (int i=1; i<length(); i++) res->append( getLogRet(i) );
    return res;
}

double Datarow::computeAverage(int N) {
    double a = 0;
    for (int i=0; i<N; i++) a += data[data.size()-1-i];
    return a/data.size();
}

double Datarow::computeDeviation(double average, int N) {
    double dv = 0;
    for (int i=0; i<N; i++) {
        double d = data[data.size()-1-i];
        dv += (d-average)*(d-average);
    }
    return sqrt(dv/(data.size()-1));
}

double Datarow::computeCovariance(DatarowPtr other, double average1, double average2) {
    size_t oDN = other->data.size();
    int N = min(data.size(), oDN); // if not same length, align the ends! (best for time series)
    double cov = 0;
    for (int i=0; i<N; i++) {
        double d1 = data[data.size()-1-i]; // read from last element
        double d2 = other->data[oDN-1-i]; // read from last element
        cov += (d1-average1)*(d2-average2);
    }
    return cov / (N-1);
}

double Datarow::computeCorrelation(DatarowPtr other) {
    int N = min(data.size(), other->data.size()); // if not same length, align the ends! (best for time series)

    double avg1 = computeAverage(N);
    double avg2 = other->computeAverage(N);

    double dev1 = computeDeviation(avg1, N);
    if (dev1 == 0) return 0;
    double dev2 = other->computeDeviation(avg2, N);
    if (dev2 == 0) return 0;

    double cov = computeCovariance(other, avg1, avg2);
    return cov/dev1/dev2;
}
