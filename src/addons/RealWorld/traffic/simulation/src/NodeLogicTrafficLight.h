#ifndef NODELOGICTRAFFICLIGHT_H
#define NODELOGICTRAFFICLIGHT_H

#include <set>
#include <vector>

using namespace std;

#include "timer.h"
#include "NodeLogic.h"

/**
 A node logic that represents a traffic light.
 This node logic might be referenced by multiple nodes which all share one timing
 of their signals.
 */
class NodeLogicTrafficLight : public NodeLogic {

    public:

        /// The states a single traffic light can be in.
        enum STATE {
            /// The vehicles can drive through.
            GREEN,
            /// The vehicles should stop.
            AMBER,
            /// The vehicles must stop.
            RED,
            /// Light is switching from red to green.
            RED_AMBER,
            /// There is no traffic light on this lane
            NONE
        };

        /// A structure to represent a light post.
        struct LightPost {
            /// The id of the node the post stands at.
            ID node;
            /// The id of the node the post is looking at.
            ID facing;
            /// The id of the street the light post is standing at.
            ID street;
            /// 1 if the light is facing in forward direction of the street, -1 if otherwise
            int direction;
            /// The colors of the lights at the different lanes.
            vector<STATE> laneStates;
        };

    private:

        /// The light posts controlled by this traffic light system.
        vector<LightPost> lightPosts;

        /// The duration of the green phase in milliseconds.
        static const int PHASE_DURATION_GREEN;
        /// The duration of the amber phase in milliseconds.
        static const int PHASE_DURATION_AMBER;
        /// The duration of the red-amber phase in milliseconds.
        static const int PHASE_DURATION_RED_AMBER;
        /// The duration of the phase where all traffic lights show red in milliseconds.
        static const int PHASE_DURATION_ALL_RED;

        /// A list of phases.
        /// Each phase consists of the start time && a list of <lightPost-IDs, laneNumber> which should be switched to their next state.
        vector< pair<int, vector< pair<size_t, size_t> > > > phases;

        /// The time that is needed to run once through all phases.
        int circleTime;

        /// The time the system has been in the current phase.
        int timeInPhase;

        /// An index inside \c phases which points to the current phase.
        size_t currentPhase;

        /// A pointer to the RoadSystem this logic is part of.
        const RoadSystem *roadSystem;

        /// The center of this node, interpolated from the positions of all nodes.
        Vec2f center;

        /// The maximal distance from the center of the logic to one of its nodes.
        double radius;

        /// A set with the IDs of the controlled traffic lights.
        set<ID> trafficLights;

        /**
         Switches the lights depending on the current phase.
         It does not change \c currentPhase but only applies the changes that are listed in \c phases.
         */
        void applyPhase();

        /**
         Finds the lightpost at a given position.
         Searchs inside the lightPosts-vector for a LightPost matching the given parameters.
         If no matching LightPos could be found, lightPosts.size() is returned.
         @param node The id of the node of the post.
         @param street The id of the street.
         @return The position of the post inside the vector || lightPosts.size().
         */
        size_t getLightPost(const ID node, const ID street) const;

        /**
         Adds a node to this traffic light.
         @param node A pointer to the node to add.
         */
        void addNode(const Node *node);

        /**
         Creates the LightPost data structure for a node/street combination.
         @param node A pointer to the node.
         @param street A pointer to the street.
         @return \c True if a post has been added, \c false if it already existed.
         */
        bool createLightPost(const Node* node, const Street* street);

        /**
         Removes the LightPost of a node/street combination.
         @param node A pointer to the node.
         @param street A pointer to the street.
         @return \c True if a post has been removed, \c false if there has be no matching post.
         */
        bool removeLightPost(const Node* node, const Street* street);

        /**
         Rebuilds the light phases of all nodes.
         Oppositing traffic lights share their phases to create realistic switches.
         */
        void rebuildLightPhases();

    protected:
        /**
         Creates an object of this class.
         Can only be called indirect over makeNodeLogic().
         @param roadSystem The RoadSystem this logic is part of.
         @param node The first node with a traffic light.
         @param radius The search radius for merging with other nodes.
         */
        NodeLogicTrafficLight(const RoadSystem *roadSystem, const Node *node, const double radius);

    public:
        /**
         Returns a NodeLogic-object for a node.
         Depending on the node && the subclass through which this method is invoked,
         either a new object might be created || an object from a nearby node will be
         used, too.
         @param roadSystem The RoadSystem this logic is part of.
         @param nodeId The id of the node to create a NodeLogic for.
         @return A pointer to an object.
         */
        static NodeLogic* makeNodeLogic(const RoadSystem *roadSystem, const ID nodeId);
        virtual ~NodeLogicTrafficLight() { };
        virtual void tick();
        virtual Vec2f getPosition() const;
        virtual void addStreet(const Node* node, const Street* street);
        virtual void removeStreet(const Node* node, const Street* street);
        virtual int canEnter(const Node* node, const ID streetId, const int lane, const ID nextStreetId, const int nextLane) const;
        virtual string toString(const bool extendedOutput) const;

        /**
         Returns the traffic lights controlled by this logic.
         @return The set with the node IDs.
         */
        const set<ID>& getTrafficLights() const;

        /**
         Returns the radius of this node logic.
         The radius is the maximal distance from the center
         of the logic to one of its nodes.
         @return The radius.
         */
        double getRadius() const;

        /**
         Returns the colors of the traffic lights.
         @param node The node the post is standing at.
         @return A vector containing one LightPost structure for every light post near the requested node.
            If the node if not part of this NodeLogicTrafficLight, an empty vector will be returned.
         */
        vector<LightPost> getLightPostState(const ID node) const;
};

#endif // NODELOGICTRAFFICLIGHT_H


