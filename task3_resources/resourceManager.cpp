// =====================================================================
//  Task 3 - Collection Counter and Delivery Resource Management Module
//  task3_resources/resourceManager.cpp
//
//  Sections:
//    1. Local helpers
//    2. Construction and file handling
//    3. Register and maintain resources      (Requirement 1, 4)
//    4. Availability views                   (Requirement 2)
//    5. Assignment, release                  (Requirement 3, 5)
//    6. Menu                                 (resourceMenu)
// =====================================================================
#include "resourceManager.hpp"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace std;
using namespace common;
using namespace resourceDefs;

// =====================================================================
//  1. Local helpers
// =====================================================================
namespace
{
void printTableHeader()
{
    cout << left << setw(10) << "ID" << setw(26) << "Type" << setw(20) << "Location"
         << setw(14) << "Status" << setw(14) << "Current Item" << "Transactions Handled\n";
    cout << string(100, '-') << "\n";
}

void printTableRow(const Resource &r)
{
    cout << left << setw(10) << r.id << setw(26) << r.type << setw(20) << r.location
         << setw(14) << r.status << setw(14) << r.currentItem << r.transactions << "\n";
}

// Splits one data-file line "id|type|location|status|item|transactions" into a
// Resource and validates every field. Returns false for a malformed line.
bool parseRecord(const string &line, Resource &record)
{
    stringstream fields(line);
    string transactionText;
    getline(fields, record.id, '|');
    getline(fields, record.type, '|');
    getline(fields, record.location, '|');
    getline(fields, record.status, '|');
    getline(fields, record.currentItem, '|');
    getline(fields, transactionText, '|');

    record.id = toUpper(trim(record.id));
    record.type = trim(record.type);
    record.location = trim(record.location);
    record.status = trim(record.status);
    record.currentItem = trim(record.currentItem);
    transactionText = trim(transactionText);

    if (!isValidId(record.id) || typeIndex(record.type) < 0 || statusIndex(record.status) < 0)
        return false;
    if (record.location.empty() || record.currentItem.empty())
        return false;
    if (transactionText.empty() || transactionText.size() > 8 ||
        transactionText.find_first_not_of("0123456789") != string::npos)
        return false;

    record.transactions = stoi(transactionText);

    // An Occupied resource must hold an item; any other status must not.
    bool isOccupied = (record.status == STATUS_OCCUPIED);
    bool holdsItem = (record.currentItem != NO_ITEM);
    return isOccupied == holdsItem;
}
} // namespace

// =====================================================================
//  2. Construction and file handling
// =====================================================================
ResourceManager::ResourceManager(const string &path) : resourceCount(0), filePath(path)
{
    load();
}

void ResourceManager::logActivity(const string &action, const string &details) const
{
    if (logger)
        logger(action, details);
}

// Linear search by ID (normalised to upper case). Returns -1 when absent.
int ResourceManager::indexOf(const string &id) const
{
    string key = toUpper(trim(id));
    for (int i = 0; i < resourceCount; i++)
        if (resources[i].id == key)
            return i;
    return -1;
}

// Refills every circular queue from the array (used after a removal).
void ResourceManager::rebuildRotation()
{
    for (int i = 0; i < TYPE_COUNT; i++)
        rotationQueues[i].clear();
    for (int i = 0; i < resourceCount; i++)
        rotationQueues[typeIndex(resources[i].type)].enqueue(resources[i].id);
}

// Loads the data file. Bad, duplicate or over-capacity lines are skipped with a
// message; a missing file is not an error (the list simply starts empty).
bool ResourceManager::load()
{
    resourceCount = 0;
    for (int i = 0; i < TYPE_COUNT; i++)
        rotationQueues[i].clear();

    ifstream inputFile(filePath);
    if (!inputFile)
        return false;

    string line;
    int lineNumber = 0, skippedLines = 0;
    while (getline(inputFile, line))
    {
        lineNumber++;
        line = trim(line);
        if (line.empty() || line[0] == '#')
            continue;

        Resource record;
        int typePos = -1;
        bool lineIsValid = parseRecord(line, record);
        if (lineIsValid)
        {
            typePos = typeIndex(record.type);
            lineIsValid = indexOf(record.id) < 0 && !rotationQueues[typePos].isFull();
        }
        if (!lineIsValid)
        {
            cout << "  [resources] Skipped invalid line " << lineNumber << " in " << filePath << "\n";
            skippedLines++;
            continue;
        }
        resources[resourceCount++] = record;
        rotationQueues[typePos].enqueue(record.id);
    }
    return skippedLines == 0;
}

// Writes every record to the data file (creates the folder on first use).
bool ResourceManager::save() const
{
    ofstream outputFile(filePath);
    if (!outputFile)
    {
        makeParentDir(filePath);
        outputFile.open(filePath);
        if (!outputFile)
            return false;
    }
    for (int i = 0; i < resourceCount; i++)
    {
        const Resource &r = resources[i];
        outputFile << r.id << '|' << r.type << '|' << r.location << '|' << r.status << '|'
                   << r.currentItem << '|' << r.transactions << '\n';
    }
    return true;
}

// =====================================================================
//  3. Register and maintain resources
// =====================================================================
Resource *ResourceManager::find(const string &id)
{
    int pos = indexOf(id);
    return pos < 0 ? nullptr : &resources[pos];
}

bool ResourceManager::registerResource(const Resource &newResource, string &err)
{
    Resource record = newResource;
    record.id = toUpper(trim(record.id));
    record.location = trim(record.location);
    int typePos = typeIndex(record.type);

    if (!isValidId(record.id))
        err = "Invalid Resource ID (1-15 letters, digits, '-' or '_').";
    else if (indexOf(record.id) >= 0)
        err = "Duplicate Resource ID: " + record.id + " already exists.";
    else if (typePos < 0)
        err = "Invalid resource type.";
    else if (!isSettableStatus(record.status))
        err = "A new resource must start as Available, Unavailable or Maintenance.";
    else if (needsLocation(record.type) && (record.location.empty() || record.location == NO_LOCATION))
        err = "Location is required for this resource type.";
    else if (record.location.empty() || record.location.find('|') != string::npos)
        err = "Invalid location.";
    else if (rotationQueues[typePos].isFull())
        err = "Maximum number of resources reached for this type.";
    else
    {
        record.currentItem = NO_ITEM;
        record.transactions = 0;
        resources[resourceCount++] = record;
        rotationQueues[typePos].enqueue(record.id);
        logActivity("REGISTER", record.id + "|" + record.type);
        if (save())
            return true;
        err = "Registered, but could not write " + filePath + ".";
    }
    return false;
}

// Changes the status of an idle resource. Occupied is only set by assign().
bool ResourceManager::updateStatus(const string &id, const string &newStatus, string &err)
{
    Resource *target = find(id);
    if (target == nullptr)
        err = "Resource not found.";
    else if (!isSettableStatus(newStatus))
        err = "Status can only be set to Available, Unavailable or Maintenance. "
              "Occupied is set by assigning a parcel/request.";
    else if (target->status == STATUS_OCCUPIED)
        err = "Resource is occupied by " + target->currentItem + ". Release it first.";
    else if (target->status == newStatus)
        err = "Resource is already " + newStatus + ".";
    else
    {
        string oldStatus = target->status;
        target->status = newStatus;
        logActivity("STATUS", target->id + "|" + oldStatus + "|" + newStatus);
        save();
        return true;
    }
    return false;
}

// A resource goes down while handling an item: free the item, keep data consistent.
bool ResourceManager::interruptResource(const string &id, const string &newStatus,
                                        string &displacedItem, string &err)
{
    Resource *target = find(id);
    if (target == nullptr)
        err = "Resource not found.";
    else if (!isDownStatus(newStatus))
        err = "A resource in use can only be interrupted to Unavailable or Maintenance.";
    else if (target->status != STATUS_OCCUPIED)
        err = "Resource is not occupied. Use a normal status update.";
    else
    {
        displacedItem = target->currentItem;
        target->status = newStatus;
        target->currentItem = NO_ITEM; // transactions NOT incremented: job not completed
        logActivity("INTERRUPT", target->id + "|" + displacedItem + "|" + newStatus);
        save();
        return true;
    }
    return false;
}

// Removes an idle resource: later records shift one place left to close the gap.
bool ResourceManager::remove(const string &id, string &err)
{
    int pos = indexOf(id);
    if (pos < 0)
        err = "Resource not found.";
    else if (resources[pos].status == STATUS_OCCUPIED)
        err = "Cannot remove an occupied resource. Release it first.";
    else
    {
        string removedId = resources[pos].id;
        for (int i = pos; i < resourceCount - 1; i++)
            resources[i] = resources[i + 1];
        resourceCount--;
        rebuildRotation();
        logActivity("REMOVE", removedId);
        save();
        return true;
    }
    return false;
}

// =====================================================================
//  4. Availability views
// =====================================================================
void ResourceManager::displayOne(const Resource &r) const
{
    cout << "\n";
    printTableHeader();
    printTableRow(r);
}

void ResourceManager::displayAll() const
{
    if (resourceCount == 0)
    {
        cout << "\n  No resources registered.\n";
        return;
    }
    cout << "\n";
    printTableHeader();
    for (int i = 0; i < resourceCount; i++)
        printTableRow(resources[i]);
    cout << "\nTotal: " << resourceCount << " resource(s)\n";
}

// Groups resources by status and shows the assigned item for occupied ones.
void ResourceManager::displayAvailability() const
{
    if (resourceCount == 0)
    {
        cout << "\n  No resources registered.\n";
        return;
    }

    int statusCounts[STATUS_COUNT] = {0, 0, 0, 0};
    for (int i = 0; i < resourceCount; i++)
        statusCounts[statusIndex(resources[i].status)]++;

    cout << "\n=========== RESOURCE AVAILABILITY ===========\n";
    for (int s = 0; s < STATUS_COUNT; s++)
        cout << " " << STATUS_NAMES[s] << ": " << statusCounts[s] << (s < STATUS_COUNT - 1 ? "   " : "\n");

    for (int s = 0; s < STATUS_COUNT; s++)
    {
        cout << "\n--- " << STATUS_NAMES[s] << " (" << statusCounts[s] << ") ---\n";
        if (statusCounts[s] == 0)
        {
            cout << "  (none)\n";
            continue;
        }
        printTableHeader();
        for (int i = 0; i < resourceCount; i++)
            if (resources[i].status == STATUS_NAMES[s])
                printTableRow(resources[i]);
    }
}

// =====================================================================
//  5. Assignment, release
// =====================================================================

// Assigns an item to one chosen resource (must be Available, item not already active).
bool ResourceManager::assign(const string &resId, const string &itemIdInput, string &err)
{
    string itemId = toUpper(trim(itemIdInput));
    Resource *target = find(resId);
    Resource *holder = nullptr; // resource already holding this item, if any
    for (int i = 0; i < resourceCount; i++)
        if (resources[i].currentItem == itemId)
            holder = &resources[i];

    if (!isValidId(itemId) || itemId == NO_ITEM)
        err = "Invalid Parcel/Request ID.";
    else if (target == nullptr)
        err = "Resource not found.";
    else if (target->status != STATUS_AVAILABLE)
        err = "Resource is not available (current status: " + target->status + ").";
    else if (holder != nullptr) // duplicate active assignment
        err = itemId + " is already assigned to " + holder->id + ".";
    else
    {
        target->status = STATUS_OCCUPIED;
        target->currentItem = itemId;
        logActivity("ASSIGN", target->id + "|" + itemId);
        save();
        return true;
    }
    return false;
}

// Round-robin allocation: take the next resource of this type from its circular
// queue, put it back at the end (rotation), and use the first Available one.
string ResourceManager::assignNext(const string &type, const string &itemIdInput, string &err)
{
    int typePos = typeIndex(type);
    if (typePos < 0)
    {
        err = "Invalid resource type.";
        return "";
    }
    ResourceCircularQueue &queue = rotationQueues[typePos];
    if (queue.isEmpty())
    {
        err = "No resources registered for this type.";
        return "";
    }

    int resourcesToCheck = queue.size();
    for (int i = 0; i < resourcesToCheck; i++)
    {
        string candidateId;
        queue.dequeue(candidateId);
        queue.enqueue(candidateId); // rotate to the back

        Resource *candidate = find(candidateId);
        if (candidate != nullptr && candidate->status == STATUS_AVAILABLE)
            return assign(candidateId, itemIdInput, err) ? candidateId : ""; // err set by assign
    }
    err = "No available " + type + " right now.";
    return "";
}

// Finishes the current job: resource becomes Available and the transaction is counted.
bool ResourceManager::release(const string &resId, string &err)
{
    Resource *target = find(resId);
    if (target == nullptr)
        err = "Resource not found.";
    else if (target->status != STATUS_OCCUPIED)
        err = "Resource is not occupied, nothing to release.";
    else
    {
        string finishedItem = target->currentItem;
        target->status = STATUS_AVAILABLE;
        target->currentItem = NO_ITEM;
        target->transactions++;
        logActivity("RELEASE", target->id + "|" + finishedItem);
        save();
        return true;
    }
    return false;
}

// Shared instance
ResourceManager &getResourceManager()
{
    static ResourceManager manager("data/resources.txt");
    return manager;
}

// =====================================================================
//  6. Menu (one small function per option)
// =====================================================================
namespace
{
// Lets the user pick one of the four resource types ("" if input closed).
string pickResourceType(const string &title)
{
    return pickFromList(title, TYPE_NAMES, TYPE_COUNT);
}

// Prints the success text or "Error: ..." for an operation result.
void report(bool success, const string &successText, const string &err)
{
    cout << "\n  " << (success ? successText : "Error: " + err) << "\n";
}

// ---- option 1 ----
void registerFlow(ResourceManager &manager)
{
    cout << "\n--- Register New Resource ---\n";
    Resource newResource;
    newResource.id = readLine(" Resource ID (e.g. CC01): ");
    if (cin.eof())
        return;
    newResource.type = pickResourceType("Resource type:");
    if (newResource.type.empty())
        return;
    newResource.location = needsLocation(newResource.type) ? readLine(" Location: ") : NO_LOCATION;
    newResource.status = pickFromList("Initial status:", SETTABLE_STATUSES, 3);
    if (newResource.status.empty())
        return;

    string err;
    bool ok = manager.registerResource(newResource, err);
    report(ok, "Resource registered successfully.", err);
}

// ---- option 4 ----
void assignToChosenFlow(ResourceManager &manager)
{
    string itemId = readLine("\n Parcel/Request ID to assign (e.g. PCL001 or REQ001): ");
    string resourceId = readLine(" Resource ID: ");
    string err;
    bool ok = manager.assign(resourceId, itemId, err);
    report(ok, "Assigned " + toUpper(itemId) + " to " + toUpper(resourceId) + ".", err);
}

void assignToNextFlow(ResourceManager &manager)
{
    string itemId = readLine("\n Parcel/Request ID to assign (e.g. PCL001 or REQ001): ");
    string type = pickResourceType("Resource type needed:");
    if (type.empty())
        return;
    string err;
    string resourceId = manager.assignNext(type, itemId, err);
    report(!resourceId.empty(),
           "Assigned " + toUpper(itemId) + " to " + resourceId + " (next in rotation).", err);
}

void assignMenuFlow(ResourceManager &manager)
{
    const string modes[2] = {"Choose a specific resource",
                             "Next available of a type (round-robin)"};
    string mode = pickFromList("Assignment mode:", modes, 2);
    if (mode == modes[0])
        assignToChosenFlow(manager);
    else if (mode == modes[1])
        assignToNextFlow(manager);
}

// ---- option 5 ----
void releaseFlow(ResourceManager &manager)
{
    string id = readLine("\n Resource ID to release: ");
    string err;
    bool ok = manager.release(id, err);
    report(ok, "Resource released and set to Available.", err);
}

// ---- option 6 ----
// The resource goes down while it is handling a parcel/request.
void interruptFlow(ResourceManager &manager, const Resource &target)
{
    string resourceId = target.id, item = target.currentItem, type = target.type;
    cout << "\n  This resource is currently handling " << item << ".\n";
    string newStatus = pickFromList("Mark as (this interrupts the current assignment):",
                                    DOWN_STATUSES, 2);
    if (newStatus.empty())
        return;
    if (!readYesNo(" Unassign " + item + " and mark " + resourceId + " as " + newStatus + "?"))
    {
        cout << "  Cancelled.\n";
        return;
    }

    string displacedItem, err;
    if (!manager.interruptResource(resourceId, newStatus, displacedItem, err))
    {
        report(false, "", err);
        return;
    }
    cout << "\n  " << resourceId << " is now " << newStatus << ". " << displacedItem
         << " has been unassigned (back to pending).\n";

    if (!readYesNo(" Reassign " + displacedItem + " to another " + type + " now?"))
    {
        cout << "  " << displacedItem << " stays pending.\n";
        return;
    }
    string newResourceId = manager.assignNext(type, displacedItem, err);
    if (newResourceId.empty())
        cout << "\n  Could not reassign: " << err << "\n  " << displacedItem
             << " stays pending until a resource is free.\n";
    else
        cout << "\n  Reassigned " << displacedItem << " to " << newResourceId << ".\n";
}

void updateFlow(ResourceManager &manager)
{
    string id = readLine("\n Enter Resource ID to update: ");
    Resource *target = manager.find(id);
    if (target == nullptr)
    {
        cout << "  Resource not found.\n";
        return;
    }
    manager.displayOne(*target);

    if (target->status == STATUS_OCCUPIED)
    {
        interruptFlow(manager, *target);
        return;
    }
    string newStatus = pickFromList("New status:", SETTABLE_STATUSES, 3);
    if (newStatus.empty())
        return;
    string err;
    bool ok = manager.updateStatus(id, newStatus, err);
    report(ok, "Status updated to " + newStatus + ".", err);
}

// ---- option 7 ----
void removeFlow(ResourceManager &manager)
{
    string id = readLine("\n Resource ID to remove: ");
    Resource *target = manager.find(id);
    if (target == nullptr)
    {
        cout << "  Resource not found.\n";
        return;
    }
    manager.displayOne(*target);
    if (!readYesNo(" Remove this resource?"))
    {
        cout << "  Cancelled.\n";
        return;
    }
    string err;
    bool ok = manager.remove(id, err);
    report(ok, "Resource removed.", err);
}
} // namespace

// Menu entry point called by main.cpp
void resourceMenu()
{
    ResourceManager &manager = getResourceManager();
    int choice = -1;
    do
    {
        cout << "\n--- RESOURCES ---\n";
        cout << " 1. Register resource\n";
        cout << " 2. Display all resources\n";
        cout << " 3. Display availability\n";
        cout << " 4. Assign parcel/request\n";
        cout << " 5. Release resource\n";
        cout << " 6. Mark available/unavailable\n";
        cout << " 7. Remove resource\n";
        cout << " 0. Back\n";
        cout << "----------------------------------------------\n";
        choice = readInt(" Enter choice: ", 0, 7);
        if (cin.eof())
            return;

        switch (choice)
        {
        case 1: registerFlow(manager); break;
        case 2: manager.displayAll(); break;
        case 3: manager.displayAvailability(); break;
        case 4: assignMenuFlow(manager); break;
        case 5: releaseFlow(manager); break;
        case 6: updateFlow(manager); break;
        case 7: removeFlow(manager); break;
        case 0: break;
        }
    } while (choice != 0);
}