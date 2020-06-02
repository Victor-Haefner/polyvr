#ifndef DATAROW_H_INCLUDED
#define DATAROW_H_INCLUDED

#include "VRMathFwd.h"

#include <OpenSG/OSGConfig.h>
#include <vector>

using namespace std;
OSG_BEGIN_NAMESPACE;

class Datarow {
    private:
        vector<double> data;

    public:
        Datarow();
        ~Datarow();

        static DatarowPtr create();

        size_t length();

        void setData(vector<double>& data);
        vector<double>& getData();

        void append(double d);
        void set(double d, int i);
        double get(int i);

        void resize(int N, double v);
        void add(DatarowPtr d);

        double getPCT(int i);
        double getLogRet(int i);
        DatarowPtr getPCTs();
        DatarowPtr getLogRets();

        double computeAverage(int N);
        double computeDeviation(double average, int N);

        double computeCovariance(DatarowPtr other, double average1, double average2);
        double computeCorrelation(DatarowPtr other);
};

OSG_END_NAMESPACE;

#endif // DATAROW_H_INCLUDED
