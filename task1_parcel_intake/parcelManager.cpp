// =====================================================================
//  parcelManager.cpp - Task 1 logic + menu
//
//  Parcel status workflow (allowed transitions):
//
//   Pending          --> Ready for Collection | Out for Delivery | Special Handling
//                        | Return Pending | On Hold | Missing | Cancelled
//   Processing /
//   Special Handling --> Ready | Out | Return Pending | On Hold | Missing | Cancelled
//                        (Processing may also go straight to Completed)
//   Ready for Coll.  --> Completed | On Hold | Missing | Return Pending | Cancelled
//   Out for Delivery --> Completed | Ready (failed delivery) | On Hold | Missing
//                        | Return Pending | Cancelled
//   Return Pending   --> Returned | On Hold | Missing
//   On Hold          --> Pending (re-queued) | Return Pending | Missing | Cancelled
//   Missing          --> Pending (found, re-queued) | Cancelled
//   Completed / Returned / Cancelled are FINAL (no changes allowed).
//
//  Invariant: a parcel is in the 'waiting' queue  <=>  status == "Pending".
//  applyStatus() (and processNext) are the only places that change a
//  status, so the queue and the list can never disagree.
// =====================================================================
#include "parcelManager.hpp"

#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "../common/Utils.hpp"   // common::readLine, readInt, readYesNo, ...

using namespace parcelDefs;

// ---------------------------------------------------------------------
//  Status transition table
// ---------------------------------------------------------------------
int parcelAllowedTransitions(const std::string &from, std::string out[])
{
    int n = 0;
    if (from == ST_PENDING)
    {
        out[n++] = ST_READY;   out[n++] = ST_OUT;           out[n++] = ST_SPECIAL;
        out[n++] = ST_RETURN_PENDING; out[n++] = ST_HOLD;
        out[n++] = ST_MISSING; out[n++] = ST_CANCELLED;
    }
    else if (from == ST_PROCESSING || from == ST_SPECIAL)
    {
        out[n++] = ST_READY;   out[n++] = ST_OUT;           out[n++] = ST_RETURN_PENDING;
        out[n++] = ST_HOLD;    out[n++] = ST_MISSING;       out[n++] = ST_CANCELLED;
        if (from == ST_PROCESSING) out[n++] = ST_COMPLETED;
    }
    else if (from == ST_READY)
    {
        out[n++] = ST_COMPLETED; out[n++] = ST_HOLD;        out[n++] = ST_MISSING;
        out[n++] = ST_RETURN_PENDING; out[n++] = ST_CANCELLED;
    }
    else if (from == ST_OUT)
    {
        out[n++] = ST_COMPLETED; out[n++] = ST_READY;       out[n++] = ST_HOLD;
        out[n++] = ST_MISSING; out[n++] = ST_RETURN_PENDING; out[n++] = ST_CANCELLED;
    }
    else if (from == ST_RETURN_PENDING)
    {
        out[n++] = ST_RETURNED; out[n++] = ST_HOLD;         out[n++] = ST_MISSING;
    }
    else if (from == ST_HOLD)
    {
        out[n++] = ST_PENDING; out[n++] = ST_RETURN_PENDING;
        out[n++] = ST_MISSING; out[n++] = ST_CANCELLED;
    }
    else if (from == ST_MISSING)
    {
        out[n++] = ST_PENDING; out[n++] = ST_CANCELLED;
    }
    return n;                       // final / unknown statuses: nothing allowed
}

bool parcelCanTransition(const std::string &from, const std::string &to)
{
    std::string allowed[PARCEL_STATUS_COUNT];
    int n = parcelAllowedTransitions(from, allowed);
    for (int i = 0; i < n; i++)
        if (allowed[i] == to)
            return true;
    return false;
}

// ---------------------------------------------------------------------
//  Field validators (prefixed names: everything is built as one
//  translation unit, so they must not clash with other tasks)
// ---------------------------------------------------------------------
static bool parcelAllAlnum(const std::string &s)
{
    for (size_t i = 0; i < s.size(); i++)
        if (!std::isalnum((unsigned char)s[i])) return false;
    return !s.empty();
}

static bool parcelValidName(const std::string &s)
{
    if (s.size() < 2 || s.size() > 50) return false;
    bool hasLetter = false;
    for (size_t i = 0; i < s.size(); i++)
    {
        unsigned char c = (unsigned char)s[i];
        if (std::isalpha(c)) hasLetter = true;
        else if (!(c == ' ' || c == '.' || c == '\'' || c == '-' || c == '/'))
            return false;                   // digits, '|' etc. rejected
    }
    return hasLetter;
}

static bool parcelValidRecipientId(const std::string &s)
{
    return s.size() >= 5 && s.size() <= 12 && parcelAllAlnum(s);
}

static bool parcelValidDeliveryInfo(const std::string &s)
{
    return !s.empty() && s.size() <= 60 && s.find('|') == std::string::npos;
}

static bool parcelValidType(const std::string &s)
{
    for (int i = 0; i < PARCEL_TYPE_COUNT; i++)
        if (PARCEL_TYPE_NAMES[i] == s) return true;
    return false;
}

// ---------------------------------------------------------------------
//  ParcelManager - core logic
// ---------------------------------------------------------------------
ParcelManager::ParcelManager(const std::string &file)
    : nextIdNumber(1), hook(), dataFile(file), saveWarned(false)
{
    loadFromFile();
}

std::string ParcelManager::generateParcelId()
{
    std::string id;
    do
    {
        std::ostringstream oss;
        oss << "PCL" << std::setw(3) << std::setfill('0') << nextIdNumber++;
        id = oss.str();
    } while (parcels.findById(id) != 0);    // guarantees uniqueness
    return id;
}

void ParcelManager::logEvent(const std::string &action, const std::string &parcelId,
                             const std::string &oldStatus, const std::string &newStatus) const
{
    if (hook)
        hook(action, parcelId + "|" + oldStatus + "|" + newStatus);
}

// Changes a status and keeps the waiting queue in sync.
void ParcelManager::applyStatus(Parcel &p, const std::string &newStatus,
                                const std::string &action, bool notify)
{
    std::string old = p.status;

    if (old == ST_PENDING && newStatus != ST_PENDING)
        waiting.removeId(p.parcelID);
    if (newStatus == ST_PENDING && old != ST_PENDING)
        waiting.enqueue(p.parcelID);

    p.status = newStatus;

    if (notify)
        logEvent(action, p.parcelID, old, newStatus);
    saveToFile();
}

bool ParcelManager::registerParcel(Parcel &p, std::string &error)
{
    // --- trim / normalise ---
    p.recipientName = common::trim(p.recipientName);
    p.recipientID   = common::toUpper(common::trim(p.recipientID));
    p.parcelType    = common::trim(p.parcelType);
    p.deliveryInfo  = common::trim(p.deliveryInfo);

    // --- missing information ---
    if (p.recipientName.empty() || p.recipientID.empty() ||
        p.parcelType.empty() || p.deliveryInfo.empty())
    {
        error = "Incomplete parcel information: all fields are required.";
        return false;
    }
    // --- invalid recipient / field information ---
    if (!parcelValidName(p.recipientName))
    { error = "Invalid recipient name (letters, spaces . ' - / only, 2-50 chars)."; return false; }
    if (!parcelValidRecipientId(p.recipientID))
    { error = "Invalid recipient ID (5-12 letters/digits, e.g. TP012345)."; return false; }
    if (!parcelValidType(p.parcelType))
    { error = "Invalid parcel type."; return false; }
    if (!parcelValidDeliveryInfo(p.deliveryInfo))
    { error = "Invalid delivery information (1-60 chars, no '|')."; return false; }

    // --- duplicate registration: the same recipient, type and delivery
    //     info while that parcel is still active ---
    for (ParcelListNode *n = parcels.first(); n != 0; n = n->next)
    {
        const Parcel &q = n->data;
        if (q.recipientID == p.recipientID && q.parcelType == p.parcelType &&
            q.deliveryInfo == p.deliveryInfo && !parcelIsFinalStatus(q.status))
        {
            error = "Duplicate registration: an active identical parcel already exists (" +
                    q.parcelID + ").";
            return false;
        }
    }

    // --- assign system fields and store ---
    p.parcelID        = generateParcelId();
    p.arrivalDateTime = common::currentDateTime();
    p.status          = ST_PENDING;

    parcels.append(p);
    waiting.enqueue(p.parcelID);

    logEvent("PARCEL_REGISTER", p.parcelID, "-", p.status);
    saveToFile();
    return true;
}

Parcel *ParcelManager::findParcel(const std::string &parcelId) const
{
    return parcels.findById(common::toUpper(common::trim(parcelId)));
}

bool ParcelManager::peekNext(Parcel *&p) const
{
    std::string id;
    if (!waiting.peek(id))
        return false;
    p = parcels.findById(id);
    return p != 0;
}

// Sort/route: take the OLDEST waiting parcel and send it to its next stage.
bool ParcelManager::processNext(const std::string &routeTo, Parcel &result,
                                std::string &error)
{
    if (waiting.isEmpty())
    {
        error = "There are no parcels waiting to be processed (queue is empty).";
        return false;
    }

    std::string id;
    waiting.peek(id);
    Parcel *p = parcels.findById(id);
    if (p == 0)                              // should never happen; self-heal
    {
        waiting.dequeue(id);
        error = "Queue referred to a missing record (" + id + ") and was cleaned up.";
        return false;
    }
    if (parcelIsFinalStatus(p->status))
    {
        error = "Parcel " + id + " is already completed.";
        return false;
    }
    if (!parcelCanTransition(p->status, routeTo))
    {
        error = "Parcel cannot be routed to that stage from its current status.";
        return false;
    }

    waiting.dequeue(id);                     // FIFO: remove the front parcel
    std::string old = p->status;
    p->status = routeTo;                     // already out of the queue
    logEvent("PARCEL_ROUTE", p->parcelID, old, routeTo);
    saveToFile();

    result = *p;
    return true;
}

bool ParcelManager::updateStatus(const std::string &parcelId,
                                 const std::string &newStatus,
                                 const std::string &action, std::string &error)
{
    Parcel *p = findParcel(parcelId);
    if (p == 0)
    {
        error = "Parcel '" + parcelId + "' does not exist.";
        return false;
    }
    if (parcelIsFinalStatus(p->status))
    {
        error = "Parcel is already " + p->status +
                " - completed parcels cannot be changed or processed again.";
        return false;
    }
    if (p->status == newStatus)
    {
        error = "Parcel already has that status.";
        return false;
    }
    if (!parcelCanTransition(p->status, newStatus))
    {
        error = "Invalid operation: " + p->status + " -> " + newStatus + " is not allowed.";
        return false;
    }
    applyStatus(*p, newStatus, action, true);
    return true;
}

bool ParcelManager::removeRecord(const std::string &parcelId, std::string &error)
{
    Parcel *p = findParcel(parcelId);
    if (p == 0)
    {
        error = "Parcel '" + parcelId + "' does not exist.";
        return false;
    }
    if (p->status != ST_PENDING && p->status != ST_CANCELLED)
    {
        error = "Only Pending (entered by mistake) or Cancelled parcels can be removed. "
                "Current status: " + p->status;
        return false;
    }
    std::string id = p->parcelID;
    std::string oldStatus = p->status;
    waiting.removeId(id);
    parcels.removeById(id);                  // p is invalid after this line
    logEvent("PARCEL_REMOVE", id, oldStatus, "-");
    saveToFile();
    return true;
}

bool ParcelManager::restoreStatus(const std::string &parcelId,
                                  const std::string &oldStatus, std::string &error)
{
    Parcel *p = findParcel(parcelId);
    if (p == 0)
    {
        error = "Parcel '" + parcelId + "' does not exist.";
        return false;
    }
    if (!parcelIsKnownStatus(oldStatus))
    {
        error = "Invalid status to restore.";
        return false;
    }
    applyStatus(*p, oldStatus, "PARCEL_UNDO", false);
    return true;
}

// ---------------------------------------------------------------------
//  Display
// ---------------------------------------------------------------------
static void parcelPrintHeader(bool withPosition)
{
    std::cout << std::left;
    if (withPosition) std::cout << std::setw(4) << "#";
    std::cout << std::setw(8)  << "ID"
              << std::setw(11) << "Rcpt ID"
              << std::setw(19) << "Recipient"
              << std::setw(17) << "Type"
              << std::setw(17) << "Arrived"
              << std::setw(22) << "Status"
              << "Delivery info\n"
              << std::string(105, '-') << "\n";
}

static void parcelPrintRow(const Parcel &p, int position = 0)
{
    std::string name = p.recipientName;
    if (name.size() > 17) name = name.substr(0, 16) + ".";
    std::cout << std::left;
    if (position > 0) std::cout << std::setw(4) << position;
    std::cout << std::setw(8)  << p.parcelID
              << std::setw(11) << p.recipientID
              << std::setw(19) << name
              << std::setw(17) << p.parcelType
              << std::setw(17) << p.arrivalDateTime
              << std::setw(22) << p.status
              << p.deliveryInfo << "\n";
}

void ParcelManager::displayAll() const
{
    if (parcels.isEmpty())
    {
        std::cout << "\nNo parcel records have been registered yet.\n";
        return;
    }
    std::cout << "\nAll parcels (" << parcels.size() << "):\n";
    parcelPrintHeader(false);
    for (ParcelListNode *n = parcels.first(); n != 0; n = n->next)
        parcelPrintRow(n->data);
}

void ParcelManager::displayWaiting() const
{
    if (waiting.isEmpty())
    {
        std::cout << "\nNo parcels are waiting to be processed.\n";
        return;
    }
    std::cout << "\nWaiting queue (" << waiting.size()
              << ") - #1 is processed first:\n";
    parcelPrintHeader(true);
    int pos = 1;
    for (const ParcelQueueNode *q = waiting.front(); q != 0; q = q->next, pos++)
    {
        Parcel *p = parcels.findById(q->parcelId);
        if (p != 0)
            parcelPrintRow(*p, pos);
    }
}

void ParcelManager::displayByRecipient(const std::string &recipientId) const
{
    std::string key = common::toUpper(common::trim(recipientId));
    int found = 0;
    for (ParcelListNode *n = parcels.first(); n != 0; n = n->next)
    {
        if (n->data.recipientID == key)
        {
            if (found == 0)
            {
                std::cout << "\nParcels for recipient " << key << ":\n";
                parcelPrintHeader(false);
            }
            parcelPrintRow(n->data);
            found++;
        }
    }
    if (found == 0)
        std::cout << "\n[Not found] No parcels for recipient '" << key << "'.\n";
}

void ParcelManager::displayDetails(const Parcel &p)
{
    std::cout << "\n------ Parcel " << p.parcelID << " ------\n"
              << " Recipient     : " << p.recipientName << " (" << p.recipientID << ")\n"
              << " Parcel type   : " << p.parcelType    << "\n"
              << " Delivery info : " << p.deliveryInfo  << "\n"
              << " Arrival time  : " << p.arrivalDateTime << "\n"
              << " Status        : " << p.status        << "\n";
}

// ---------------------------------------------------------------------
//  File handling
//  Format: parcelID|recipientID|recipientName|parcelType|arrival|status|deliveryInfo
// ---------------------------------------------------------------------
bool ParcelManager::saveToFile() const
{
    common::makeParentDir(dataFile);
    std::ofstream out(dataFile.c_str());
    if (!out)
    {
        if (!saveWarned)
        {
            std::cout << "[Warning] Could not write " << dataFile
                      << ". Changes are kept in memory only.\n";
            saveWarned = true;
        }
        return false;
    }
    for (ParcelListNode *n = parcels.first(); n != 0; n = n->next)
    {
        const Parcel &p = n->data;
        out << p.parcelID << '|' << p.recipientID << '|' << p.recipientName << '|'
            << p.parcelType << '|' << p.arrivalDateTime << '|' << p.status << '|'
            << p.deliveryInfo << '\n';
    }
    return true;
}

bool ParcelManager::loadFromFile()
{
    std::ifstream in(dataFile.c_str());
    if (!in)
        return false;                        // first run: no file yet

    std::string line;
    int skipped = 0;
    while (std::getline(in, line))
    {
        line = common::trim(line);
        if (line.empty()) continue;

        const int FIELDS = 7;
        std::string f[FIELDS];
        size_t start = 0;
        int k = 0;
        while (k < FIELDS)
        {
            size_t bar = line.find('|', start);
            if (bar == std::string::npos || k == FIELDS - 1)
            { f[k++] = line.substr(start); break; }   // last field keeps the rest
            f[k++] = line.substr(start, bar - start);
            start = bar + 1;
        }

        Parcel p;
        p.parcelID = f[0];     p.recipientID = f[1];  p.recipientName = f[2];
        p.parcelType = f[3];   p.arrivalDateTime = f[4];
        p.status = f[5];       p.deliveryInfo = f[6];

        if (k < FIELDS || p.parcelID.empty() || !parcelIsKnownStatus(p.status) ||
            parcels.findById(p.parcelID) != 0)
        { skipped++; continue; }             // corrupt / duplicate line

        parcels.append(p);
        if (p.status == ST_PENDING)
            waiting.enqueue(p.parcelID);     // file order = arrival order

        // keep the ID counter ahead of every loaded ID ("PCL007" -> 8)
        if (p.parcelID.size() > 3)
        {
            int num = std::atoi(p.parcelID.c_str() + 3);
            if (num >= nextIdNumber) nextIdNumber = num + 1;
        }
    }
    if (skipped > 0)
        std::cout << "[Warning] " << skipped << " invalid line(s) in " << dataFile
                  << " were skipped.\n";
    return true;
}

// ---------------------------------------------------------------------
//  Shared instance
// ---------------------------------------------------------------------
ParcelManager &getParcelManager()
{
    static ParcelManager instance;
    return instance;
}

// =====================================================================
//  MENU (user interface for Task 1)
// =====================================================================

// Reads one trimmed line. Returns false on end-of-input.
static bool parcelReadLine(const std::string &prompt, std::string &out)
{
    out = common::readLine(prompt);
    return !std::cin.eof();
}

// Keeps asking until the validator accepts. "0" cancels.
static bool parcelAsk(const std::string &prompt, bool (*valid)(const std::string &),
                      const std::string &errMsg, std::string &out)
{
    while (true)
    {
        if (!parcelReadLine(prompt, out) || out == "0")
            return false;
        if (valid(out))
            return true;
        std::cout << "  ! " << errMsg << " (enter 0 to cancel)\n";
    }
}

static void parcelMenuRegisterParcel()
{
    std::cout << "\n--- Register New Parcel (enter 0 at a text prompt to cancel) ---\n";
    Parcel p;
    if (!parcelAsk(" Recipient name          : ", parcelValidName,
                   "letters and spaces only", p.recipientName)) { std::cout << "Cancelled.\n"; return; }
    if (!parcelAsk(" Recipient ID (TP/staff) : ", parcelValidRecipientId,
                   "5-12 letters/digits, e.g. TP012345", p.recipientID)) { std::cout << "Cancelled.\n"; return; }
    if (!parcelAsk(" Delivery info (e.g. Block A Lobby): ", parcelValidDeliveryInfo,
                   "cannot be empty or contain '|'", p.deliveryInfo)) { std::cout << "Cancelled.\n"; return; }

    p.parcelType = common::pickFromList("Parcel type:", PARCEL_TYPE_NAMES, PARCEL_TYPE_COUNT);
    if (p.parcelType.empty()) return;        // input closed

    std::string err;
    if (getParcelManager().registerParcel(p, err))
    {
        std::cout << "\nParcel registered successfully.\n";
        ParcelManager::displayDetails(p);
        std::cout << " (added to the processing queue, position "
                  << getParcelManager().waitingCount() << ")\n";
    }
    else
        std::cout << "\n[Error] " << err << "\n";
}

static void parcelMenuSearch()
{
    ParcelManager &pm = getParcelManager();
    std::cout << "\n--- Search Parcel ---\n"
              << " 1. By Parcel ID\n 2. By Recipient ID\n";
    int c = common::readInt(" Choice: ", 1, 2);
    if (std::cin.eof()) return;

    std::string key;
    if (!parcelReadLine(" Enter search key: ", key)) return;
    if (key.empty()) { std::cout << "[Error] Search key cannot be empty.\n"; return; }
    if (pm.totalParcels() == 0) { std::cout << "\nNo parcel records exist yet.\n"; return; }

    if (c == 2) { pm.displayByRecipient(key); return; }

    Parcel *p = pm.findParcel(key);
    if (p == 0) std::cout << "\n[Not found] No parcel matches '" << key << "'.\n";
    else        ParcelManager::displayDetails(*p);
}

static void parcelMenuProcessNext()
{
    ParcelManager &pm = getParcelManager();
    Parcel *front = 0;
    if (!pm.peekNext(front))
    {
        std::cout << "\n[Error] There are no parcels waiting to be processed.\n";
        return;
    }
    std::cout << "\n--- Sort / Process Next Parcel (front of queue) ---";
    ParcelManager::displayDetails(*front);
    std::cout << "\n Route this parcel to:\n"
              << "  1. Recipient collection\n"
              << "  2. Campus delivery\n"
              << "  3. Special handling\n"
              << "  4. Return to sender\n"
              << "  5. Hold for investigation\n"
              << "  0. Leave in queue\n";
    int c = common::readInt(" Choice: ", 0, 5);
    if (c == 0 || std::cin.eof()) { std::cout << "Parcel left in the queue.\n"; return; }

    const std::string routes[6] = {ST_PENDING, ST_READY, ST_OUT, ST_SPECIAL,
                                   ST_RETURN_PENDING, ST_HOLD};
    Parcel result;
    std::string err;
    if (pm.processNext(routes[c], result, err))
        std::cout << "\nParcel " << result.parcelID << " routed -> " << result.status << "\n";
    else
        std::cout << "\n[Error] " << err << "\n";
}

static void parcelMenuUpdateStatus()
{
    ParcelManager &pm = getParcelManager();
    if (pm.totalParcels() == 0) { std::cout << "\n[Error] No parcel records exist.\n"; return; }

    std::string id;
    if (!parcelReadLine("\n Parcel ID to update: ", id)) return;
    Parcel *p = pm.findParcel(id);
    if (p == 0) { std::cout << "[Error] Parcel '" << id << "' does not exist.\n"; return; }

    ParcelManager::displayDetails(*p);
    std::string options[PARCEL_STATUS_COUNT];
    int n = parcelAllowedTransitions(p->status, options);
    if (n == 0)
    {
        std::cout << "\n[Error] Parcel is already " << p->status
                  << " - no further changes are allowed.\n";
        return;
    }
    std::cout << "\n Change status to:\n";
    for (int i = 0; i < n; i++)
        std::cout << "  " << (i + 1) << ". " << options[i] << "\n";
    std::cout << "  0. Cancel\n";
    int c = common::readInt(" Choice: ", 0, n);
    if (c == 0 || std::cin.eof()) return;

    if (options[c - 1] == ST_CANCELLED &&
        !common::readYesNo(" Cancelling is permanent. Continue?"))
        return;

    std::string err;
    const std::string target = options[c - 1];
    if (pm.updateStatus(p->parcelID, target, "PARCEL_STATUS", err))
        std::cout << "\nStatus updated to " << target << ".\n";
    else
        std::cout << "\n[Error] " << err << "\n";
}

static void parcelMenuQuickStatus(const std::string &target, const char *actionCode)
{
    ParcelManager &pm = getParcelManager();
    std::string id;
    if (!parcelReadLine("\n Parcel ID to mark as " + target + ": ", id)) return;
    if (id.empty()) { std::cout << "[Error] Parcel ID cannot be empty.\n"; return; }
    if (target == ST_CANCELLED && !common::readYesNo(" Cancelling is permanent. Continue?"))
        return;
    std::string err;
    if (pm.updateStatus(id, target, actionCode, err))
        std::cout << "\nParcel is now " << target << ".\n";
    else
        std::cout << "\n[Error] " << err << "\n";
}

static void parcelMenuRemove()
{
    ParcelManager &pm = getParcelManager();
    std::string id;
    if (!parcelReadLine("\n Parcel ID to remove: ", id)) return;
    if (id.empty()) { std::cout << "[Error] Parcel ID cannot be empty.\n"; return; }
    Parcel *p = pm.findParcel(id);
    if (p == 0) { std::cout << "[Error] Parcel '" << id << "' does not exist.\n"; return; }
    ParcelManager::displayDetails(*p);
    if (!common::readYesNo(" Permanently remove this record?")) return;
    std::string err;
    if (pm.removeRecord(id, err)) std::cout << "\nRecord removed.\n";
    else                          std::cout << "\n[Error] " << err << "\n";
}

void parcelManagerMenu()
{
    int choice;
    do
    {
        std::cout << "\n==============================================\n"
                  << " TASK 1 - PARCEL INTAKE, SORTING & PROCESSING\n"
                  << "==============================================\n"
                  << " 1. Register new parcel\n"
                  << " 2. Display all parcels\n"
                  << " 3. Search / view parcel\n"
                  << " 4. Display waiting queue\n"
                  << " 5. Sort / process next parcel\n"
                  << " 6. Update parcel status\n"
                  << " 7. Report parcel missing\n"
                  << " 8. Cancel parcel\n"
                  << " 9. Remove parcel record\n"
                  << " 0. Back to main menu\n"
                  << "----------------------------------------------\n";
        choice = common::readInt(" Enter choice: ", 0, 9);
        if (std::cin.eof()) return;

        switch (choice)
        {
        case 1: parcelMenuRegisterParcel();                          break;
        case 2: getParcelManager().displayAll();               break;
        case 3: parcelMenuSearch();                                  break;
        case 4: getParcelManager().displayWaiting();           break;
        case 5: parcelMenuProcessNext();                             break;
        case 6: parcelMenuUpdateStatus();                            break;
        case 7: parcelMenuQuickStatus(ST_MISSING, "PARCEL_MISSING"); break;
        case 8: parcelMenuQuickStatus(ST_CANCELLED, "PARCEL_CANCEL"); break;
        case 9: parcelMenuRemove();                                  break;
        }
    } while (choice != 0);
}