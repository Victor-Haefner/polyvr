#ifndef VRFUNCTION_H_INCLUDED
#define VRFUNCTION_H_INCLUDED

#include <boost/function.hpp>
#include <iostream>
#include <boost/exception/exception.hpp>
#include "VRName.h"
#include <memory>

using namespace std;


class VRFunction_base : public VRName_base {
    private:
        int prof_id = 0;

    protected:
        void t0();
        void t1();

        void printExcept(boost::exception& e);
};

template<class T, class R>
class VRFunction : public VRFunction_base {
    boost::function<R (T)> fkt;
    public:
        VRFunction(string name, boost::function<R (T)> fkt) : fkt(fkt) { this->name = name; }
        ~VRFunction() {}

        R operator()(T t) {
            R res;
            try {
                if (fkt) {
                    t0();
                    res = fkt(t);
                    t1();
                }
            } catch (boost::exception& e) { printExcept(e); }
            return res;
        }

        static std::shared_ptr<VRFunction<T> > create(string name, boost::function<R (T)> fkt) {
            return std::shared_ptr<VRFunction<T> >( new VRFunction<T>(name, fkt) );
        }
};

template<class T>
class VRFunction<T, void> : public VRFunction_base {
    boost::function<void (T)> fkt;
    public:
        VRFunction(string name, boost::function<void (T)> fkt) : fkt(fkt) { this->name = name; }
        ~VRFunction() {}

        void operator()(T t) {
            try {
                if (fkt) {
                    t0();
                    fkt(t);
                    t1();
                }
            } catch (boost::exception& e) { printExcept(e); }
        }

        static std::shared_ptr<VRFunction<T> > create(string name, boost::function<void (T)> fkt) {
            return std::shared_ptr<VRFunction<T> >( new VRFunction<T>(name, fkt) );
        }
};

#endif // VRFUNCTION_H_INCLUDED
