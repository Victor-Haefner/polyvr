#ifndef VRCODECOMPLETION_H_INCLUDED
#define VRCODECOMPLETION_H_INCLUDED

#include <map>
#include <vector>

using namespace std;

class VRPyCodeCompletion {
    private:
        bool vrModMapInitiated = false;
        map<string, vector<string> > vrModMap;

        void initVRModMap();

	public:
        VRPyCodeCompletion();
        ~VRPyCodeCompletion();

        vector<string> getSuggestions(string s);
};

#endif // VRCODECOMPLETION_H_INCLUDED
