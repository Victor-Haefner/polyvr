#ifndef VRGUIFWD_H_INCLUDED
#define VRGUIFWD_H_INCLUDED

#include "core/utils/VRFwdDeclTemplate.h"

namespace OSG {

ptrFwd(VRGuiTreeExplorer);
ptrFwd(VRConsoleWidget);
ptrFwd(VRSemanticWidget);
ptrFwd(VRConceptWidget);
ptrFwd(VREntityWidget);
ptrFwd(VRRuleWidget);
ptrFwd(VRConnectorWidget);
ptrFwd(VRVisualLayer);
ptrFwd(VRSignal);

}

namespace Gtk {

ptrFwd(Window);
ptrFwd(ToggleToolButton);
ptrFwd(ScrolledWindow);
ptrFwd(Notebook);
ptrFwd(Label);
ptrFwd(TextBuffer);
ptrFwd(TextTag);

}

#endif // VRGUIFWD_H_INCLUDED
