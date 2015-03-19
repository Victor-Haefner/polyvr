#ifndef VRFUNCTION_H_INCLUDED
#define VRFUNCTION_H_INCLUDED

#include <boost/function.hpp>
#include <iostream>
#include <boost/exception/all.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include "VRName.h"

using namespace std;


class VRFunction_base : public VRName {
    ;
};

template<typename T>
class VRFunction : public VRFunction_base {
    boost::function<void (T)> fkt;
    public:
        VRFunction(string _name, boost::function<void (T)> _fkt) : fkt(_fkt) { name = _name; }

        void operator()(T t) {
            try { if(fkt) fkt(t); }
            catch (boost::exception& e) {
				cout << "VRFunction::() exception occured: " << boost::diagnostic_information(e) << endl;
            }
        }
};

#endif // VRFUNCTION_H_INCLUDED
