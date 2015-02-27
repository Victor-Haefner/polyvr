#ifndef NETWORKINTERFACE_H
#define NETWORKINTERFACE_H

#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_types.hpp>

#include "JsonServer.h"

class TrafficSimulator;

/// A class that handles a simple JSON-HTTP server to communicate with the traffic simulator over the network.
class NetworkInterface : public IJsonServerListener {

    private:
        /// No copy constructing.
        NetworkInterface(const NetworkInterface&);
        /// No assignment of objects.
        /// @return The modified object. || not.
        NetworkInterface& operator=(const NetworkInterface&);

        /// An object of the JsonServer class which is the real server.
        JsonServer server;

        /// A mutex to synchronize access to the RoadSystem.
        boost::mutex mutex;
        /// A condition variable to synchronize access to the RoadSystem.
        boost::condition_variable condition;
        /// The number of threads/network requests currently waiting.
        int waitingCount;

        /// A pointer to the TrafficSimulator that should be modified.
        /// Is temporary set by applyChanges() if the threads should work.
        TrafficSimulator *simulator;

        // Overrides from the IJsonServerListener
        // You do not have to care about these.
        friend class JsonServer;
        virtual const Value getViewareaData(const unsigned int id);
#ifdef FILE_LOADING
    public: // Needed to allow the thread to add the file contents as network input
#endif // FILE_LOADING
        virtual const Value handlePostRequest(const Value& input);
        virtual SIMULATORSTATE getSimulatorState();

    public:

        /// Creates the server.
        /// Does not start the server on construction.
        NetworkInterface();

        /// Starts the server.
        void start();

        /// Stops the server.
        void stop();

        /**
         Returns whether the server is running at the moment.
         @return \c True if the server is running.
         */
        bool isRunning();

        /**
         Applies the changes received over the network to the given simulator.
         @param trafficSimulator The simulator to modify.
         */
        void applyChanges(TrafficSimulator *trafficSimulator);

};

#endif // NETWORKINTERFACE_H
