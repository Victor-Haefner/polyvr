#include "VREncryption.h"

string fixString(string ciphertext) {
    string result;
    for (unsigned int i = 0; i < ciphertext.size(); i++) {
        char buffer[2];
        sprintf (buffer, "%02x", ciphertext[i]);
        result += string(buffer, 2);
    }
    return result;
}

string encrypt(string plaintext, string key, string iv) {
    cout << "Encrypt " << plaintext.size() << " bytes" << endl << endl;
    string ciphertext;
    CryptoPP::AES::Encryption aesEncryption((const byte*)key.c_str(), key.size());
    CryptoPP::CBC_Mode_ExternalCipher::Encryption cbcEncryption( aesEncryption, (const byte*)iv.c_str() );
    CryptoPP::StreamTransformationFilter stfEncryptor(cbcEncryption, new CryptoPP::StringSink( ciphertext ) );
    stfEncryptor.Put( reinterpret_cast<const unsigned char*>( plaintext.c_str() ), plaintext.length() + 1 );
    stfEncryptor.MessageEnd();
    //cout << "Cipher text (" << ciphertext.size() << " bytes) '" << ciphertext << "'" << endl << endl;
    //cout << "Cipher text '" << fixString(ciphertext) << "'" << endl << endl;
    return ciphertext;
}

string decrypt(string ciphertext, string key, string iv) {
    cout << "Decrypt " << ciphertext.size() << " bytes" << endl << endl;
    string decryptedtext;
    CryptoPP::AES::Decryption aesDecryption((const byte*)key.c_str(), key.size());
    CryptoPP::CBC_Mode_ExternalCipher::Decryption cbcDecryption( aesDecryption, (const byte*)iv.c_str() );
    CryptoPP::StreamTransformationFilter stfDecryptor(cbcDecryption, new CryptoPP::StringSink( decryptedtext ) );
    stfDecryptor.Put( reinterpret_cast<const unsigned char*>( ciphertext.c_str() ), ciphertext.size() );
    stfDecryptor.MessageEnd();
    //cout << "Decrypted text: '" << decryptedtext << "'" << endl << endl;
    return decryptedtext;
}

void compressFolder(string folder, string archive) {
    cout << "Compress folder " << folder << " to " << archive << endl << endl;
    Zipper z(archive);
    z.add(folder);
}

void uncompressFolder(string archive, string folder) {
    cout << "Uncompress archive " << archive << " to " << folder << endl << endl;
    Unzipper z(archive);
    z.extract(folder);
}

string loadAsString(string path) {
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

void storeBin(string data, string path) {
    ofstream output(path, ios::out | ios::binary);
	output.write(data.c_str(), data.size());
	output.flush();
	output.close();
}

void test1() {
    string key = "123456789012345678901234567890ab";
    string iv = "0000000000000000";

    string plaintext = "This is super secret nonsense, just for you!";
    string ciphertext = encrypt(plaintext, key, iv);
    string decryptext = decrypt(ciphertext, key, iv);
}

void test2() {
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
