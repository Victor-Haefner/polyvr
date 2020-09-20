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
        void insert(int i, double v);

        vector<double> getMinMax();

        double getPCT(int i);
        double getLogRet(int i);
        DatarowPtr getPCTs();
        DatarowPtr getLogRets();

        double computeAverage(int from = 0, int to = -1);
        double computeDeviation(double average, int from = 0, int to = -1);

        double computeCovariance(DatarowPtr other, double average1, double average2, int offset1 = 0, int offset2 = 0);
        double computeCorrelation(DatarowPtr other, int offset1 = 0, int offset2 = 0);
};

OSG_END_NAMESPACE;

#endif // DATAROW_H_INCLUDED
