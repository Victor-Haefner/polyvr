#ifndef VRATTACHMENT_H_INCLUDED
#define VRATTACHMENT_H_INCLUDED

#include <string>

using namespace std;

class VRAttachment {
    private:
        struct base {
            virtual ~base();
            virtual string asString() = 0;
        };

        template<typename T>
        struct attachment : base {
            T data;
            attachment(T& t);
            ~attachment();
            string asString();
        };

        base* data = 0;

    public:
        VRAttachment();
        ~VRAttachment();

        template<typename T>
        void set(T& t);

        template<typename T>
        T get();

        string asString();
};

#endif // VRATTACHMENT_H_INCLUDED
