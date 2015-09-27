#ifndef VRFURNITURE_H_INCLUDED
#define VRFURNITURE_H_INCLUDED


OSG_BEGIN_NAMESPACE;
using namespace std;

class VRFurnitureManager {

    //map<VRScene*, vector<string>* > types;
    vector<string>* types;

    private:

        VRFurnitureManager() { types = new vector<string>(); }

        void operator= (VRFurnitureManager v) { ; }

        ~VRFurnitureManager() { ; }

    public:
        static VRFurnitureManager* get() {
            static VRFurnitureManager* singleton_opt = new VRFurnitureManager();
            return singleton_opt;
        }

        void addType(string s) { types->push_back(s); }

        void parseGraph(VRScene* scene, VRObjectPtr o) {
            //if (types.count(scene) == 0) return;
            if (types == 0) return;
            string name = o->getName();

            //for (uint i=0;i<types[scene]->size();i++) {
                //if (name.find(types[scene]->at(i)) != string::npos) {
                    //scene->addToGroup(o, types[scene]->at(i), false);
            for (uint i=0;i<types->size();i++) {
                if (name.find(types->at(i)) != string::npos) {
                    vector<VRObjectPtr>* geos = o->getObjectListByType("Geometry");
                    for (uint j=0;j<geos->size();j++) {
                        VRGeometryPtr geo = static_pointer_cast<VRGeometry>((geos->at(j));
                        scene->addToGroup(geo, types->at(i), false);
                    }
                }
            }

            for (int i=0;i<o->getChildrenCount();i++)
                parseGraph(scene, o->getChild(i));
        }
};

OSG_END_NAMESPACE;

#endif // VRFURNITURE_H_INCLUDED
