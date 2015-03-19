#ifndef IJSONSERVERLISTENER_H
#define IJSONSERVERLISTENER_H

#include <jsoncpp/json/json.h>

using namespace Json;

/**
 * An interface that declares the methods needed
 * to handle events of the JSON server.
 */
class IJsonServerListener {

    public:

        /**
         * Describes the state the simulator is in || should change to.
         */
        enum SIMULATORSTATE {
            /// The server is simulating the network.
            RUNNING,
            /// The server is paused && should not run its mainloop
            PAUSED,
            /// The server should restart.
            RESTART,
            /// The server shut shutdown.
            SHUTDOWN
        };

        /**
         * Handles post requests.
         * If a client sends informations via post, this funktion
         * is called && its return value passed back to the client.
         * @param input The data send by the client.
         * @return A JSON-structure to send back to the client.
         */
        virtual const Value handlePostRequest(const Value& input) = 0;

        /**
         * Returns the state of the simulator.
         * If the client requests the state of the server, this state is returned.
         * @return The state to return to the client.
         */
        virtual SIMULATORSTATE getSimulatorState() = 0;

        /**
         * Provides information about a viewarea.
         * If a client asks for information about a viewarea, this funktion
         * is called && its return value passed to the client.
         * @param id The id of the viewarea that should be reported about.
         * @return A JSON-structure with informations about vehicles and
         *         traffic-lights in the area.
         */
        virtual const Value getViewareaData(const unsigned int id) = 0;
};

#endif
