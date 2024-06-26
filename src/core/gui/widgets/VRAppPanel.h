#ifndef VRAPPPANEL_H_INCLUDED
#define VRAPPPANEL_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/utils/VRFwdDeclTemplate.h"
#include "core/utils/VRName.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

ptrFwd(VRAppLauncher);
ptrFwd(VRAppPanel);
ptrFwd(VRAppManager);

class VRAppPanel : public std::enable_shared_from_this<VRAppPanel>, public VRName {
    private:
        map<string, VRAppLauncherPtr> apps;

    public:
        VRAppPanel(string name);
        ~VRAppPanel();
        static VRAppPanelPtr create(string name);
        VRAppPanelPtr ptr();

        VRAppLauncherPtr addLauncher(string path, string timestamp, VRAppManager* mgr, bool write_protected, bool favorite, string table);
        void remLauncher(string path);
        VRAppLauncherPtr getLauncher(string path);
        map<string, VRAppLauncherPtr> getLaunchers();
        int getSize();

        void fillTable(string t, int& i);
        void clearTable(string t);
        void setGuiState(VRAppLauncherPtr e, bool running, bool noLauncherScene);
};

OSG_END_NAMESPACE;

#endif // VRAPPPANEL_H_INCLUDED
