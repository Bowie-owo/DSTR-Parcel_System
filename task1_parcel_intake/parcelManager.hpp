// =====================================================================
//  parcelManager.hpp - Task 1: Parcel Intake, Sorting and Processing
//  Combines ParcelList (all records) + ParcelQueue (waiting to be
//  processed) and exposes an API the other tasks can call.
//  Uses the shared Parcel struct (common/Parcel.hpp) and the shared
//  data file data/parcels.txt.
// =====================================================================
#ifndef PARCEL_MANAGER_HPP
#define PARCEL_MANAGER_HPP

#include <string>
#include "../common/Parcel.hpp"
#include "../common/Utils.hpp"   // common::ActivityLogger
#include "parcelQueue.hpp"   // ParcelList + ParcelQueue

// ---------------------------------------------------------------------
//  Status / type strings stored in data/parcels.txt (same style as
//  resourceDefs in Resource.hpp). "Pending", "Processing", "Completed",
//  "Cancelled" and "Returned" match the existing data file.
// ---------------------------------------------------------------------
namespace parcelDefs
{
const int PARCEL_STATUS_COUNT = 11;
const int PARCEL_TYPE_COUNT = 5;

const std::string ST_PENDING = "Pending";                // registered, waiting in the queue
const std::string ST_PROCESSING = "Processing";
const std::string ST_READY = "Ready for Collection";
const std::string ST_OUT = "Out for Delivery";
const std::string ST_SPECIAL = "Special Handling";
const std::string ST_RETURN_PENDING = "Return Pending";
const std::string ST_HOLD = "On Hold";
const std::string ST_MISSING = "Missing";
const std::string ST_COMPLETED = "Completed";            // FINAL (collected or delivered)
const std::string ST_RETURNED = "Returned";              // FINAL
const std::string ST_CANCELLED = "Cancelled";            // FINAL

const std::string PARCEL_STATUS_NAMES[PARCEL_STATUS_COUNT] = {
    ST_PENDING, ST_PROCESSING, ST_READY, ST_OUT, ST_SPECIAL, ST_RETURN_PENDING,
    ST_HOLD, ST_MISSING, ST_COMPLETED, ST_RETURNED, ST_CANCELLED};

const std::string PARCEL_TYPE_NAMES[PARCEL_TYPE_COUNT] = {
    "Standard Parcel", "Fragile Box", "Document", "Electronics", "Clothing"};

inline bool parcelIsKnownStatus(const std::string &s)
{
    for (int i = 0; i < PARCEL_STATUS_COUNT; i++)
        if (PARCEL_STATUS_NAMES[i] == s)
            return true;
    return false;
}

// A final parcel has finished its journey and can no longer be processed.
inline bool parcelIsFinalStatus(const std::string &s)
{
    return s == ST_COMPLETED || s == ST_RETURNED || s == ST_CANCELLED;
}
} // namespace parcelDefs

// Fills 'out' (size >= parcelDefs::PARCEL_STATUS_COUNT) with the statuses
// reachable from 'from'; returns how many.
int  parcelAllowedTransitions(const std::string &from, std::string out[]);
bool parcelCanTransition(const std::string &from, const std::string &to);

class ParcelManager
{
private:
    ParcelList            parcels;       // all records
    ParcelQueue           waiting;       // "Pending" parcels, FIFO
    int                   nextIdNumber;  // next number for PCLxxx
    common::ActivityLogger hook;         // Task 4 activity history (optional)
    std::string           dataFile;
    mutable bool          saveWarned;

    std::string generateParcelId();
    void applyStatus(Parcel &p, const std::string &newStatus,
                     const std::string &action, bool notify);
    void logEvent(const std::string &action, const std::string &parcelId,
                  const std::string &oldStatus, const std::string &newStatus) const;

public:
    explicit ParcelManager(const std::string &file = "data/parcels.txt");

    // Task 4 hook. action: PARCEL_REGISTER | PARCEL_ROUTE | PARCEL_STATUS |
    // PARCEL_MISSING | PARCEL_CANCEL | PARCEL_REMOVE
    // details: "<parcelID>|<old status>|<new status>"
    void setActivityLogger(const common::ActivityLogger &h) { hook = h; }

    // ---- 1. register ----
    // p must hold recipientID, recipientName, parcelType, deliveryInfo.
    // parcelID, arrivalDateTime and status are assigned here.
    bool registerParcel(Parcel &p, std::string &error);

    // ---- 2. maintain / locate ----
    Parcel *findParcel(const std::string &parcelId) const;
    void    displayAll() const;
    void    displayWaiting() const;
    void    displayByRecipient(const std::string &recipientId) const;
    static void displayDetails(const Parcel &p);

    // ---- 3. sort / route the parcel at the front of the queue ----
    bool peekNext(Parcel *&p) const;
    bool processNext(const std::string &routeTo, Parcel &result, std::string &error);

    // ---- 4. update / remove ----
    bool updateStatus(const std::string &parcelId, const std::string &newStatus,
                      const std::string &action, std::string &error);
    bool removeRecord(const std::string &parcelId, std::string &error);

    // For Task 4 undo: force a previous status back (no activity logged).
    bool restoreStatus(const std::string &parcelId, const std::string &oldStatus,
                       std::string &error);

    int  totalParcels() const { return parcels.size(); }
    int  waitingCount() const { return waiting.size(); }

    // ---- file handling (data/parcels.txt, pipe separated) ----
    bool loadFromFile();
    bool saveToFile() const;
};

// Single shared instance (so Task 2/3/4 and the demo use the same data).
ParcelManager &getParcelManager();

// Task 1 menu loop. main.cpp's parcelMenu() calls this.
void parcelManagerMenu();

#endif