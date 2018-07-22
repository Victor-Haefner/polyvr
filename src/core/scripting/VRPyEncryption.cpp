#include "VRPyEncryption.h"
#include "VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(Encryption, New_ptr);

PyMethodDef VRPyEncryption::methods[] = {
    {"encrypt", PyWrap( Encryption, encrypt, "Run test 1", string, string, string, string ) },
    {"decrypt", PyWrap( Encryption, decrypt, "Run test 1", string, string, string, string ) },
    {"compressFolder", PyWrap( Encryption, compressFolder, "Run test 1", void, string, string ) },
    {"uncompressFolder", PyWrap( Encryption, uncompressFolder, "Run test 1", void, string, string ) },
    {"loadAsString", PyWrap( Encryption, loadAsString, "Run test 1", string, string ) },
    {"storeBin", PyWrap( Encryption, storeBin, "Run test 1", void, string, string ) },
    {"test1", PyWrap( Encryption, test1, "Run test 1", void ) },
    {"test2", PyWrap( Encryption, test2, "Run test 2", void ) },
    {NULL}  /* Sentinel */
};
