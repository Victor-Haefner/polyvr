#ifndef VRENCRYPTION_H_INCLUDED
#define VRENCRYPTION_H_INCLUDED

#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>

#include <cryptopp/modes.h>
#include <cryptopp/aes.h>
#include <cryptopp/filters.h>

#include <OpenSG/OSGConfig.h>

#include "zipper/filesystem.h"
#include "zipper/zipper.h"
#include "zipper/unzipper.h"
#include "core/utils/VRUtilsFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VREncryption {
    private:

        string fixString(string ciphertext);

    public:
        VREncryption();
        ~VREncryption();

        static VREncryptionPtr create();

        string encrypt(string plaintext, string key, string iv);
        string decrypt(string ciphertext, string key, string iv);

        void compressFolder(string folder, string archive);
        void uncompressFolder(string archive, string folder);

        string loadAsString(string path);
        void storeBin(string data, string path);

        void test1();
        void test2();
};

OSG_END_NAMESPACE;

#endif // VRENCRYPTION_H_INCLUDED
