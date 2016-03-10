#ifndef VRFUNCTION_H_INCLUDED
#define VRFUNCTION_H_INCLUDED

#include <boost/function.hpp>
#include <iostream>
#include <boost/exception/all.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include "VRName.h"
#include <memory>

using namespace std;


class VRFunction_base : public VRName_base {
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
        VRFunction(string name, boost::function<void (T)> fkt) : fkt(fkt) { this->name = name; }
        ~VRFunction() {}

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

        static std::shared_ptr<VRFunction<T> > create(string name, boost::function<void (T)> fkt) {
            return std::shared_ptr<VRFunction<T> >( new VRFunction<T>(name, fkt) );
        }
};

#endif // VRFUNCTION_H_INCLUDED
