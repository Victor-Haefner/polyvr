#include "VRMillingCuttingToolProfile.h"
#include "core/utils/toString.h"

#include <iostream>

using namespace OSG;

VRMillingCuttingToolProfile::VRMillingCuttingToolProfile() {}
VRMillingCuttingToolProfile::~VRMillingCuttingToolProfile() {}

shared_ptr<VRMillingCuttingToolProfile> VRMillingCuttingToolProfile::create() { return shared_ptr<VRMillingCuttingToolProfile>(new VRMillingCuttingToolProfile()); }

void VRMillingCuttingToolProfile::addPointProfile(Vec2d point) {
    profile.push_back(point);
}

bool VRMillingCuttingToolProfile::alreadyInProfile(float newx) {
    for (size_t i = 0; i < profile.size(); i++)
    {
        float epsilon = 1e-6;
        if (abs(newx - profile[i][0]) < epsilon)
        {
            return true;
        }
    }
    return false;
}

float VRMillingCuttingToolProfile::getLength() {
    if (profile.size() > 0) {
        return profile.back()[0];
    }
    return 0;
}

float VRMillingCuttingToolProfile::maxProfile(Vec3d toolPosition, Vec3d cubePosition, Vec3d cubeSize) {
    float py = cubePosition[1];
    float sy = cubeSize[1];
    float ptooly = toolPosition[1];

    float newx1 = py + sy/2.0f - ptooly;
    float newx2 = py - sy/2.0f - ptooly;

    int indexMax = -1;
    int indexMin = -1;
    vector<Vec2d> newList;

    if (profile.size() == 0) return 0;

    if ((newx1 > 0) && (newx2 < profile.back()[0])
            && (newx2 > 0) && (newx1 < profile.back()[0]))
    {
        int p1 = lookForNearestIndex(newx1);
        int p2 = lookForNearestIndex(newx2);

        if (alreadyInProfile(newx1) && alreadyInProfile(newx2))
        {
            for (int i = p2; i <= p1; i++)
            {
                newList.push_back(profile[i]);
            }
            return lookForMaxInList(newList);
        }
        else if (alreadyInProfile(newx1))
        {
            Vec2d newPoint2 = {newx2, newy(newx2, p2)};
            if (newx2 < profile[p2][0])
                indexMin = p2 + 1;
            else
                indexMin = p2;
            newList.push_back(newPoint2);
            for (int i = indexMin; i <= p1; i++)
                newList.push_back(profile[i]);
            return lookForMaxInList(newList);
        }
        else if (alreadyInProfile(newx2))
        {
            Vec2d newPoint1 = {newx1, newy(newx1, p1)};
            if (newx1 < profile[p1][0])
                indexMax = p1 - 1;
            else
                indexMax = p1;
            for (int i = p2; i <= indexMax; i++)
                newList.push_back(profile[i]);
            newList.push_back(newPoint1);
            return lookForMaxInList(newList);
        }

        Vec2d newPoint1 = {newx1, newy(newx1, p1)};
        Vec2d newPoint2 = {newx2, newy(newx2, p2)};

        if (newx1 < profile[p1][0])
            indexMax = p1 - 1;
        else
            indexMax = p1;
        if (newx2 < profile[p2][0])
            indexMin = p2 + 1;
        else
            indexMin = p2;

        newList.push_back(newPoint2);
        for (int i = indexMin; i <= indexMax; i++)
            newList.push_back(profile[i]);
        newList.push_back(newPoint1);
    }
    else if ((newx1 > 0) && (newx1 < profile.back()[0]))
    {
        int p = lookForNearestIndex(newx1);
        if (alreadyInProfile(newx1))
        {
            for (int i=0; i <= p; i++)
            {
                newList.push_back(profile[i]);
            }
            return lookForMaxInList(newList);
        }
        Vec2d newPoint = {newx1, newy(newx1, p)};

        if (newx1 < profile[p][0])
            indexMax = p - 1;
        else
            indexMax = p;

        for (int i = 0; i <= indexMax; i++)
            newList.push_back(profile[i]);
        newList.push_back(newPoint);
    }
    else if ((newx2 < profile.back()[0]) && (newx2 > 0))
    {
        int p = lookForNearestIndex(newx2);
        if (alreadyInProfile(newx2))
        {
            for (size_t i = p; i < profile.size(); i++)
            {
                newList.push_back(profile[i]);
            }
            return lookForMaxInList(newList);
        }
        Vec2d newPoint = {newx2, newy(newx2, p)};

        if (newx2 < profile[p][0])
            indexMin = p + 1;
        else
            indexMin = p;

        newList.push_back(newPoint);
        for (size_t i = indexMin; i < profile.size(); i++)
            newList.push_back(profile[i]);
    }
    else if ((newx1 >= profile.back()[0]) && (newx2 <= 0))
    {
        return lookForMaxInList(profile);
    }

    return lookForMaxInList(newList);
}

float VRMillingCuttingToolProfile::newy(float newx, int p) {
    float a = 0, b = 0;

    if (newx < profile[p][0])
    {
        a = (profile[p-1][1] - profile[p][1]) / (profile[p-1][0] - profile[p][0]);
        b = (profile[p-1][0] * profile[p][1] - profile[p][0] * profile[p-1][1]) / (profile[p-1][0] - profile[p][0]);
    }
    else
    {
        a = (profile[p][1] - profile[p+1][1])  / (profile[p][0] - profile[p+1][0]);
        b = (profile[p][0] * profile[p+1][1] - profile[p+1][0] * profile[p][1]) / (profile[p][0] - profile[p+1][0]);
    }
    return (a * newx + b);
}

//Add of Marie
int VRMillingCuttingToolProfile::lookForNearestIndex(float newx) {
    int index = 0;

    for(size_t i = 0; i < profile.size(); i++)
    {
        if (abs(newx - profile[i][0]) < abs(newx - profile[index][0]))
        {
            index = i;
        }
    }

    return index;
}

//Add of Marie
float VRMillingCuttingToolProfile::lookForMaxInList(vector<Vec2d> liste) {
    float maximum = 0;

    for (size_t i = 0; i < liste.size(); i++)
    {
        if (liste[i][1] > maximum)
        {
            maximum = liste[i][1];
        }
    }
    return maximum;
}

