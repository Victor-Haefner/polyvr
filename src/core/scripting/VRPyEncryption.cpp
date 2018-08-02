#include "VRPyEncryption.h"
#include "VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(Encryption, New_ptr);

PyMethodDef VRPyEncryption::methods[] = {
    {"encrypt", PyWrap( Encryption, encrypt, "Encrypt a string: plaintext, key, iv", string, string, string, string ) },
    {"decrypt", PyWrap( Encryption, decrypt, "Decrypt a string: plaintext, key, iv", string, string, string, string ) },
    {"compressFolder", PyWrap( Encryption, compressFolder, "Compress folder: folder, archive", void, string, string ) },
    {"uncompressFolder", PyWrap( Encryption, uncompressFolder, "Uncompress folder: archive, folder", void, string, string ) },
    {"loadAsString", PyWrap( Encryption, loadAsString, "Load file and return as string", string, string ) },
    {"storeBin", PyWrap( Encryption, storeBin, "Store string as a binary file: data, path", void, string, string ) },
    {"test1", PyWrap( Encryption, test1, "Run test 1", void ) },
    {"test2", PyWrap( Encryption, test2, "Run test 2", void ) },
    {NULL}  /* Sentinel */
};
