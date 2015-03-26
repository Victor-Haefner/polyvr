#ifndef VRFUNCTION_H_INCLUDED
#define VRFUNCTION_H_INCLUDED

#include <boost/function.hpp>
#include <iostream>
#include <boost/exception/all.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include "VRName.h"

using namespace std;


class VRFunction_base : public VRName {
    private:
        int prof_id = 0;

    protected:
        void t0();
        void t1();
};

template<typename T>
class VRFunction : public VRFunction_base {
    boost::function<void (T)> fkt;
    public:
        VRFunction(string _name, boost::function<void (T)> _fkt) : fkt(_fkt) { name = _name; }

        void operator()(T t) {
            try {
                if(fkt == 0) return;
                t0();
                fkt(t);
                t1();
            } catch (boost::exception& e) {
				cout << "VRFunction::() exception occured: " << boost::diagnostic_information(e) << endl;
            }
        }
};

#endif // VRFUNCTION_H_INCLUDED
