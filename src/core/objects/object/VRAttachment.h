#ifndef VRATTACHMENT_H_INCLUDED
#define VRATTACHMENT_H_INCLUDED

using namespace std;

class VRAttachment {
    private:
        struct base{};

        template<typename T>
        struct attachment : base {
            T data;
            attachment(T& t);
        };

        base* data;

    public:
        VRAttachment();

        template<typename T>
        void set(T& t);

        template<typename T>
        T get();
};

#endif // VRATTACHMENT_H_INCLUDED
