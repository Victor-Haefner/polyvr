#ifndef VRATTACHMENT_H_INCLUDED
#define VRATTACHMENT_H_INCLUDED

#include <string>
#include "core/utils/VRName.h"

using namespace std;

class VRAttachment : public OSG::VRName {
    public:
        struct base {
            virtual ~base();
            virtual string asString() = 0;
            virtual void fromString(string s) = 0;
            virtual string typeName() const = 0;
        };

        template<typename T>
        struct attachment : base {
            T data;
            attachment(T& t);
            ~attachment();
            string asString() override;
            void fromString(string s) override;
            string typeName() const override;
        };

    private:
        base* data = 0;

    public:
        VRAttachment(string name);
        virtual ~VRAttachment();

        template<typename T>
        void set(T& t);

        template<typename T>
        T get();

        string asString();
        bool fromString(string v);
};

#endif // VRATTACHMENT_H_INCLUDED
