#ifndef MODULETRAFFIC_H
#define MODULETRAFFIC_H

#include "TrafficSimulation.h"

using namespace std;

namespace realworld {

    class ModuleTraffic: public BaseModule {
        private:
            OSMMapDB* mapDB;
            TrafficSimulation* simulation;

        public:
            ModuleTraffic(OSMMapDB* mapDB, MapCoordinator* mapCoordinator, TextureManager* texManager) : BaseModule(mapCoordinator, texManager) {
                this->mapDB = mapDB;
                this->simulation = new TrafficSimulation(mapCoordinator);
            }

            ~ModuleTraffic() {
                delete simulation;
            }

            virtual string getName() { return "ModuleTraffic"; }


            TrafficSimulation *getTrafficSimulation() {
                return simulation;
            }

            virtual void loadBbox(AreaBoundingBox* bbox) {
                OSMMap* osmMap = mapDB->getMap(bbox->str);
                if (!osmMap) return;

                VRFunction<VRThread*>* func = new VRFunction<VRThread*>("trafficAddMap", boost::bind(&realworld::TrafficSimulation::addMap, simulation, osmMap));
                VRSceneManager::get()->initThread(func, "trafficAddMap", false);
            }

            virtual void unloadBbox(AreaBoundingBox* bbox) {
                OSMMap* osmMap = mapDB->getMap(bbox->str);
                if (!osmMap) return;

                // Not inside a thread since osmMap might no longer be valid after this method returns
                simulation->removeMap(osmMap);
            }

            void physicalize(bool b) {
                return;
            }

            TrafficSimulation *getSimulation() {
                return simulation;
            }
    };
}

#endif // MODULETRAFFIC_H




