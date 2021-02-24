#include "VREncryption.h"
#include "core/utils/toString.h"

#ifdef _WIN32
using namespace CryptoPP;
#endif

using namespace OSG;


VREncryption::VREncryption() {}
VREncryption::~VREncryption() {}
VREncryptionPtr VREncryption::create() { return VREncryptionPtr( new VREncryption() ); }

void fixKey(string& key) {
    int N = key.size();
    for (int i=N; i<32; i++) key += '0';
}

string VREncryption::encrypt(string plaintext, string key, string iv) {
    fixKey(key);
    cout << "Encrypt " << plaintext.size() << " bytes" << endl;
    string ciphertext;
    try {
        CryptoPP::AES::Encryption aesEncryption((const byte*)key.c_str(), key.size());
        CryptoPP::CBC_Mode_ExternalCipher::Encryption cbcEncryption( aesEncryption, (const byte*)iv.c_str() );
        CryptoPP::StreamTransformationFilter stfEncryptor(cbcEncryption, new CryptoPP::StringSink( ciphertext ), CryptoPP::StreamTransformationFilter::PKCS_PADDING );
        stfEncryptor.Put( reinterpret_cast<const unsigned char*>( plaintext.c_str() ), plaintext.size() );
        stfEncryptor.MessageEnd();
    } catch(exception& e) { cout << "VREncryption::encrypt, encryption failed with \"" << e.what() << "\"!" << endl; return ""; }
    return ciphertext;
}

string VREncryption::decrypt(string ciphertext, string key, string iv) {
    fixKey(key);
    cout << "Decrypt " << ciphertext.size() << " bytes" << endl;
    string decryptedtext;
    try {
        CryptoPP::AES::Decryption aesDecryption((const byte*)key.c_str(), key.size());
        CryptoPP::CBC_Mode_ExternalCipher::Decryption cbcDecryption( aesDecryption, (const byte*)iv.c_str() );
        CryptoPP::StreamTransformationFilter stfDecryptor(cbcDecryption, new CryptoPP::StringSink( decryptedtext ), CryptoPP::StreamTransformationFilter::PKCS_PADDING );
        stfDecryptor.Put( reinterpret_cast<const unsigned char*>( ciphertext.c_str() ), ciphertext.size() );
        stfDecryptor.MessageEnd();
    } catch(exception& e) { cout << "VREncryption::decryption, encryption failed with \"" << e.what() << "\"!" << endl; return ""; }
    return decryptedtext;
}

void VREncryption::compressFolder(string folder, string archive) {
    cout << "Compress folder " << folder << " to " << archive << endl << endl;
    Zipper z(archive);
    z.add(folder);
}

void VREncryption::uncompressFolder(string archive, string folder) {
    cout << "Uncompress archive " << archive << " to " << folder << endl << endl;
    Unzipper z(archive);
    z.extract(folder);
}

string VREncryption::loadAsString(string path) {
    ifstream input(path, ios::in | ios::binary);
    input.seekg(0, ios::end);
    int size = input.tellg();
    input.seekg(0, ios::beg);
	auto buf = new char[size];
	input.read( buf, size);
	string res = "";
	res.append(buf, size);
	delete buf;
	return res;
}

void VREncryption::storeBin(string data, string path) {
    ofstream output(path, ios::out | ios::binary);
	output.write(data.c_str(), data.size());
	output.flush();
	output.close();
}

void VREncryption::test1() {
    string key = "123456789012345678901234567890ab";
    string iv = "0000000000000000";

    string plaintext = "This is super secret nonsense, just for you!";
    string ciphertext = encrypt(plaintext, key, iv);
    string decryptext = decrypt(ciphertext, key, iv);
}

void VREncryption::test2() {
    string key = "123456789012345678901234567890ab";
    string iv = "0000000000000000";

    compressFolder("testData", "test.zip");
    string data = loadAsString("test.zip");
    string encrypted = encrypt(data, key, iv);
    string decrypted = decrypt(encrypted, key, iv);
    storeBin(decrypted, "test2.zip");
    uncompressFolder("test2.zip", "testData2");

    FILESYSTEM::remove("test.zip");
    FILESYSTEM::remove("test2.zip");
    FILESYSTEM::remove("testData2");
}
