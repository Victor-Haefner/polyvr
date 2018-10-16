#include "VRKinematics.h"

/**

Kinematics structure

- Joints
    - 6 DoF Joints
        - Ball/Hinge Joint
        - Sliding Joint

- VRKinematic
    - Graph
        - Node is VRConstraint

- Graph traversal
    - traversal priority
        - depth first
        - sibling first
    - multithreading
        - mark nodes visited by action
        - lock nodes for specific traversal action



*/
