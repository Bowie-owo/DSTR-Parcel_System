// =====================================================================
//  Task 3 - Collection Counter and Delivery Resource Management Module
//  task3_resources/resourceManager.hpp
//
//  Files used by this module:
//    ../common/Resource.hpp          Resource struct + types/statuses
//    ../common/Utils.hpp             readInt, readYesNo, ... (shared)
//    resourceCircularQueue.hpp/.cpp  hand-written circular queue
//    resourceManager.hpp/.cpp        this class + resourceMenu()
//
//  Data structures (no STL containers):
//    - ResourceCircularQueue : one per resource type, round-robin allocation
//    - plain array           : the Resource records (at most 4 x 50 = 200)
//
//  Allocation rule: a parcel/request goes to the NEXT AVAILABLE resource
//  of the requested type, in rotation (see assignNext).
// =====================================================================
#pragma once

#include <string>

#include "../common/Resource.hpp"
#include "../common/Utils.hpp"
#include "resourceCircularQueue.hpp"

class ResourceManager
{
private:
    Resource resources[resourceDefs::MAX_RESOURCES]; // record storage
    int resourceCount;                               // records in use
    ResourceCircularQueue rotationQueues[resourceDefs::TYPE_COUNT]; // one per type
    std::string filePath;
    common::ActivityLogger logger; // optional hook for Task 4

    int indexOf(const std::string &id) const; // position in the array, -1 if absent
    void rebuildRotation();
    void logActivity(const std::string &action, const std::string &details) const;

public:
    explicit ResourceManager(const std::string &path = "data/resources.txt");

    void setActivityLogger(common::ActivityLogger fn) { logger = fn; }

    // File handling
    bool load();
    bool save() const;

    // Requirement 1 and 4: register and maintain
    bool registerResource(const Resource &newResource, std::string &err);
    Resource *find(const std::string &id); // do not keep the pointer across remove()
    bool updateStatus(const std::string &id, const std::string &newStatus, std::string &err);
    bool remove(const std::string &id, std::string &err);

    // A resource going down WHILE handling an item: it becomes Unavailable or
    // Maintenance, the item is unassigned and returned in displacedItem (Task 1/2
    // put it back to Pending); no transaction is counted.
    bool interruptResource(const std::string &id, const std::string &newStatus,
                           std::string &displacedItem, std::string &err);

    // Requirement 2: availability views
    void displayAll() const;
    void displayAvailability() const;
    void displayOne(const Resource &r) const;

    // Requirement 3 and 5: assignment
    bool assign(const std::string &resId, const std::string &itemId, std::string &err);
    std::string assignNext(const std::string &type, const std::string &itemId, std::string &err);
    bool release(const std::string &resId, std::string &err);

    int count() const { return resourceCount; }
};

// Shared instance so other modules (Task 1/2/4) reuse the same data.
ResourceManager &getResourceManager();

// Menu entry point expected by main.cpp
void resourceMenu();