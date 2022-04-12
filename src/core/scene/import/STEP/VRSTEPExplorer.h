#ifndef VRSTEPEXPLORER_H_INCLUDED
#define VRSTEPEXPLORER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "../VRImportFwd.h"

#include "VRSTEP.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRSTEPExplorer : public std::enable_shared_from_this<VRSTEPExplorer> {
	private:
	    VRGuiTreeExplorerPtr treeview;
	    VRSTEPPtr stepPtr;

	    bool doIgnore(VRSTEP::Node* node);
        void translate(string& name);
        void on_explorer_select(VRGuiTreeExplorer* e);

	public:
		VRSTEPExplorer(string file);
		~VRSTEPExplorer();

		static VRSTEPExplorerPtr create(string file);
		VRSTEPExplorerPtr ptr();

        void explore(VRSTEP::Node* node, int parent = 0, bool doFilter = true);
        void traverse(VRSTEPPtr stepPtr, VRSTEP::Node* node, bool doFilter = true);
};

OSG_END_NAMESPACE;

#endif //VRSTEPEXPLORER_H_INCLUDED
