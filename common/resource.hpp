// =====================================================================
//  common/Resource.hpp
//  Resource struct shared by all modules (no data structure here), plus
//  the valid resource types / statuses and tiny one-line checks.
// =====================================================================
#pragma once

#include <string>

// One collection counter / delivery desk / delivery rider.
struct Resource
{
    std::string id;          // unique identifier, e.g. "CC01"
    std::string type;        // one of TYPE_NAMES
    std::string location;    // NO_LOCATION when not applicable
    std::string status;      // one of STATUS_NAMES
    std::string currentItem; // Parcel/Request ID being handled, NO_ITEM if none
    int transactions = 0;    // completed transactions
};

namespace resourceDefs
{
const int TYPE_COUNT = 4;
const int STATUS_COUNT = 4;
const int MAX_PER_TYPE = 50;                      // capacity of each type's circular queue
const int MAX_RESOURCES = TYPE_COUNT * MAX_PER_TYPE;

// ---- statuses ----
const std::string STATUS_AVAILABLE = "Available";
const std::string STATUS_OCCUPIED = "Occupied";
const std::string STATUS_UNAVAILABLE = "Unavailable";
const std::string STATUS_MAINTENANCE = "Maintenance";

const std::string STATUS_NAMES[STATUS_COUNT] = {STATUS_AVAILABLE, STATUS_OCCUPIED,
                                                STATUS_UNAVAILABLE, STATUS_MAINTENANCE};

// Statuses staff may set by hand (Occupied is only set by assigning an item).
const std::string SETTABLE_STATUSES[3] = {STATUS_AVAILABLE, STATUS_UNAVAILABLE, STATUS_MAINTENANCE};
// Statuses a busy resource can be interrupted to.
const std::string DOWN_STATUSES[2] = {STATUS_UNAVAILABLE, STATUS_MAINTENANCE};

// ---- types ----
const std::string TYPE_PERSONNEL = "Delivery Personnel";
const std::string TYPE_NAMES[TYPE_COUNT] = {"Collection Counter", "Delivery Desk",
                                            "Special-Handling Counter", TYPE_PERSONNEL};

// ---- placeholders stored in the data file ----
const std::string NO_ITEM = "-";
const std::string NO_LOCATION = "-";

// ---- one-line helpers (return -1 / false when invalid) ----
inline int typeIndex(const std::string &type)
{
    for (int i = 0; i < TYPE_COUNT; i++)
        if (TYPE_NAMES[i] == type)
            return i;
    return -1;
}

inline int statusIndex(const std::string &status)
{
    for (int i = 0; i < STATUS_COUNT; i++)
        if (STATUS_NAMES[i] == status)
            return i;
    return -1;
}

inline bool isSettableStatus(const std::string &s)
{
    return s == STATUS_AVAILABLE || s == STATUS_UNAVAILABLE || s == STATUS_MAINTENANCE;
}

inline bool isDownStatus(const std::string &s)
{
    return s == STATUS_UNAVAILABLE || s == STATUS_MAINTENANCE;
}

// Delivery personnel have no fixed location; every other type does.
inline bool needsLocation(const std::string &type)
{
    return type != TYPE_PERSONNEL;
}
} // namespace resourceDefs