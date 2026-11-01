// =====================================================================
//  APU SMART CAMPUS PARCEL COLLECTION AND DELIVERY MANAGEMENT SYSTEM
//  main.cpp - Main menu, module wiring and the end-to-end workflow demo
//
//  Each task menu function is implemented by the member who owns that
//  task (inside their own folder). main.cpp does three things:
//    1. shows the main menu and hands control to the chosen module
//    2. WIRES the modules together (integrationConnect) so they exchange
//       information instead of running as five separate programs
//    3. runs the 10-step end-to-end workflow from section 4 of the brief
//
//  Build from the project root in PowerShell (one file, as README.md says):
//    g++ -std=c++17 main.cpp -o main.exe
//  The implemented modules are #included at the very end of this file, so
//  do not list those .cpp files again in the build command.
// =====================================================================

#include <cstdlib>   // system()
#include <iostream>
#include <string>

#include "common/Utils.hpp" // shared input helpers: readInt, readYesNo, ...

#include "task1_parcel_intake/parcelManager.hpp"
#include "task2_service_reuqests/requestManager.hpp"
#include "task3_resources/resourceManager.hpp"
#include "task4_activity_history/activityManager.hpp"
#include "task5_parcel_tree/parcelTree.hpp"

using namespace std;
using namespace common;

// ---------------------------------------------------------------------
//  Menu entry points. Each owner implements their own in their folder:
//    Task 1 -> parcelManagerMenu()   (parcelManager.cpp)
//    Task 2 -> requestMenu()         (requestManager.cpp)
//    Task 3 -> resourceMenu()        (resourceManager.cpp)
//    Task 4 -> activityMenu()        (activityManager.cpp)
//    Task 5 -> treeMenu()            (parcelTree.cpp)
// ---------------------------------------------------------------------
void parcelMenu() { parcelManagerMenu(); }

// ---------------------------------------------------------------------
//  Small screen helpers (the input helpers live in common/Utils)
// ---------------------------------------------------------------------

// Waits for the user to press Enter so output does not scroll away.
void pauseScreen()
{
    cout << "\nPress Enter to continue...";
    string dummy;
    getline(cin, dummy);
}

// Clears the terminal (Windows or Linux/macOS).
void clearScreen()
{
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

// =====================================================================
//  MODULE WIRING
//  Every module reports its changes through a hook (setActivityLogger).
//  A hook can hold only ONE function, so each hook below calls Task 4
//  (history) first and then does that module's extra integration work.
//
//      Task 1 (parcels)   --> Task 4 history + Task 5 parcel tree
//      Task 2 (requests)  --> Task 4 history + Task 1 (parcel status)
//      Task 3 (resources) --> Task 4 history   (connectActivityLogging)
// =====================================================================

// Splits "a|b|c" into up to maxParts pieces. Returns how many were found.
static int integrationSplit(const string &text, string parts[], int maxParts)
{
    int count = 0;
    size_t start = 0;
    while (count < maxParts)
    {
        size_t bar = text.find('|', start);
        if (bar == string::npos)
        {
            parts[count++] = text.substr(start);
            break;
        }
        parts[count++] = text.substr(start, bar - start);
        start = bar + 1;
    }
    return count;
}

// Task 1 -> Task 5: keep the parcel tree in step with the parcel records.
// details = "parcelID|oldStatus|newStatus"
static void integrationSyncTree(const string &action, const string &details)
{
    string parts[3];
    if (integrationSplit(details, parts, 3) < 1)
        return;
    const string &parcelId = parts[0];
    ParcelTree &tree = getParcelTree();

    if (action == "PARCEL_REMOVE")
    {
        tree.deleteParcel(parcelId);
        return;
    }
    Parcel *parcel = getParcelManager().findParcel(parcelId);
    if (parcel == nullptr)
        return;
    if (tree.searchParcel(parcelId) == nullptr)
        tree.insertParcel(*parcel);          // new parcel (PARCEL_REGISTER) or missing from the tree
    else
        tree.updateParcel(parcelId, *parcel); // status / route / undo
}

// Task 2 -> Task 1: when a request is completed or returned, the associated parcel
// is updated too (brief, Task 2 requirement 4). details = "requestID|parcelID"
static void integrationUpdateParcelAfterRequest(const string &action, const string &details)
{
    if (action != "COMPLETE_REQUEST" && action != "RETURN_REQUEST")
        return;
    string parts[2];
    if (integrationSplit(details, parts, 2) < 2)
        return;

    ParcelManager &parcels = getParcelManager();
    Parcel *parcel = parcels.findParcel(parts[1]);
    if (parcel == nullptr)
        return;

    const string target = (action == "RETURN_REQUEST") ? parcelDefs::ST_RETURNED : parcelDefs::ST_COMPLETED;
    string err;

    // Task 1 only allows certain status changes, so go through the allowed steps:
    //   Returned  is reached from "Return Pending"
    //   Completed is reached from Ready for Collection / Out for Delivery / Processing
    if (target == parcelDefs::ST_RETURNED && parcel->status != parcelDefs::ST_RETURN_PENDING)
        parcels.updateStatus(parts[1], parcelDefs::ST_RETURN_PENDING, "PARCEL_STATUS", err);
    if (target == parcelDefs::ST_COMPLETED &&
        (parcel->status == parcelDefs::ST_PENDING || parcel->status == parcelDefs::ST_SPECIAL))
        parcels.updateStatus(parts[1], parcelDefs::ST_READY, "PARCEL_STATUS", err);

    err.clear();
    if (parcels.updateStatus(parts[1], target, "PARCEL_STATUS", err))
        cout << "  [auto] Parcel " << parts[1] << " is now " << target << ".\n";
    else
        cout << "  [auto] Parcel " << parts[1] << " stays '" << parcel->status << "': " << err << "\n";
}

// Registers every hook once, before any menu runs.
static void integrationConnect()
{
    // Task 3 -> Task 4 (function provided by the Task 4 module)
    connectActivityLogging();

    // Task 1 -> Task 4 + Task 5
    getParcelManager().setActivityLogger(
        [](const string &action, const string &details)
        {
            getActivityManager().record(action, details); // ignored while an undo is running
            integrationSyncTree(action, details);
        });

    // Task 2 -> Task 4 + Task 1
    getRequestManager().setActivityLogger(
        [](const string &action, const string &details)
        {
            getActivityManager().record(action, details);
            integrationUpdateParcelAfterRequest(action, details);
        });

    // Start-up: put the parcels saved in data/parcels.txt into the tree.
    treeLoadParcelsFromFile(getParcelTree());
}

// =====================================================================
//  END-TO-END WORKFLOW DEMO (menu option 6)
//  Runs the ten steps of section 4 of the brief on ONE new parcel, using the
//  real modules, so the lecturer can see them exchange information.
// =====================================================================
static void demoPrintStep(int number, const string &title)
{
    cout << "\n----------------------------------------------\n"
         << " Step " << number << " of 10 : " << title << "\n"
         << "----------------------------------------------\n";
}

// Asks for a value; pressing Enter accepts the suggested default.
static string demoAsk(const string &prompt, const string &defaultValue)
{
    string answer = readLine(" " + prompt + " [" + defaultValue + "]: ");
    return answer.empty() ? defaultValue : answer;
}

static bool demoContinue()
{
    if (cin.eof())
        return false;
    if (readYesNo("\n Continue to the next step?"))
        return true;
    cout << " Workflow stopped. Everything done so far is saved.\n";
    return false;
}

void runWorkflowDemo()
{
    ParcelManager &parcels = getParcelManager();
    RequestManager &requests = getRequestManager();
    ResourceManager &resources = getResourceManager();
    ParcelTree &tree = getParcelTree();
    string err;

    cout << "\n==============================================\n";
    cout << " END-TO-END WORKFLOW DEMO\n";
    cout << " One new parcel goes through all five modules.\n";
    cout << " (Press Enter at a question to accept the suggestion.)\n";
    cout << "==============================================\n";

    // ---------------- Step 1: parcel arrives ----------------
    demoPrintStep(1, "Parcel arrives at the delivery centre");
    Parcel parcel;
    parcel.recipientName = demoAsk("Recipient name", "Demo Student");
    parcel.recipientID = demoAsk("Recipient ID", "TP000001");
    parcel.deliveryInfo = demoAsk("Delivery info", "Block A Lobby");
    parcel.parcelType = pickFromList("Parcel type:", parcelDefs::PARCEL_TYPE_NAMES,
                                     parcelDefs::PARCEL_TYPE_COUNT);
    if (parcel.parcelType.empty() || !demoContinue())
        return;

    // ---------------- Step 2: register (Task 1 -> Task 4 + Task 5) ----------------
    demoPrintStep(2, "Parcel is registered and its information is maintained");
    if (!parcels.registerParcel(parcel, err))
    {
        cout << " [Error] " << err << "\n Workflow stopped.\n";
        return;
    }
    cout << " Task 1: parcel " << parcel.parcelID << " registered (" << parcel.status
         << ", waiting queue size " << parcels.waitingCount() << ").\n";
    cout << " Task 5: parcel tree now holds " << tree.size() << " record(s); "
         << parcel.parcelID << (tree.searchParcel(parcel.parcelID) ? " is in the tree.\n" : " is NOT in the tree.\n");
    cout << " Task 4: registration recorded in the activity history.\n";
    if (!demoContinue())
        return;

    // ---------------- Step 3: sort / route (Task 1) ----------------
    demoPrintStep(3, "Parcel is sorted / routed for its next action");
    const int ROUTE_COUNT = 5;
    const char *routeNames[ROUTE_COUNT] = {"Recipient collection", "Campus delivery", "Special handling",
                                           "Return to sender", "Hold for investigation"};
    const string routeStatus[ROUTE_COUNT] = {parcelDefs::ST_READY, parcelDefs::ST_OUT, parcelDefs::ST_SPECIAL,
                                             parcelDefs::ST_RETURN_PENDING, parcelDefs::ST_HOLD};
    const string serviceType[ROUTE_COUNT] = {"Recipient Collection", "Campus Delivery", "Special Handling",
                                             "Return to Sender", ""};
    const string resourceType[ROUTE_COUNT] = {resourceDefs::TYPE_NAMES[0], resourceDefs::TYPE_PERSONNEL,
                                              resourceDefs::TYPE_NAMES[2], resourceDefs::TYPE_NAMES[1], ""};
    cout << " Next action for this parcel:\n";
    for (int i = 0; i < ROUTE_COUNT; i++)
        cout << "   " << (i + 1) << ". " << routeNames[i] << "\n";
    int route = readInt(" Choice: ", 1, ROUTE_COUNT) - 1;
    if (cin.eof())
        return;
    if (!parcels.updateStatus(parcel.parcelID, routeStatus[route], "PARCEL_ROUTE", err))
    {
        cout << " [Error] " << err << "\n Workflow stopped.\n";
        return;
    }
    cout << " Task 1: parcel " << parcel.parcelID << " routed -> " << routeStatus[route] << ".\n";
    cout << " Task 5: tree record updated to '" << tree.searchParcel(parcel.parcelID)->status << "'.\n";

    if (route == ROUTE_COUNT - 1) // on hold: no request is needed, jump to the final status
    {
        demoPrintStep(10, "Final status");
        cout << " Parcel " << parcel.parcelID << " is ON HOLD for investigation.\n"
             << " No service request or resource is needed until the hold is released\n"
             << " (Task 1 > Update parcel status).\n";
        return;
    }
    if (!demoContinue())
        return;

    // ---------------- Step 4: create a service request (Task 2) ----------------
    demoPrintStep(4, "A service request is created");
    ServiceRequest request;
    request.parcelID = parcel.parcelID;
    request.recipientID = parcel.recipientID;
    request.recipientName = parcel.recipientName;
    request.serviceType = serviceType[route];
    if (!requests.createRequest(request, err))
    {
        cout << " [Error] " << err << "\n Workflow stopped.\n";
        return;
    }
    cout << " Task 2: request " << request.requestID << " (" << request.serviceType << ") created for parcel "
         << request.parcelID << ", status Pending.\n";
    if (!demoContinue())
        return;

    // ---------------- Step 5: select the request (Task 2 priority rule) ----------------
    demoPrintStep(5, "The appropriate request is selected");
    ServiceRequest next;
    if (requests.peekNextRequest(next))
    {
        cout << " Priority rule: Special Handling > Return to Sender > Re-delivery > Campus Delivery >\n"
             << " Recipient Collection; equal priority -> oldest request first.\n"
             << " The queue would hand out " << next.requestID << " (" << next.serviceType << ") first.\n";
        if (next.requestID != request.requestID)
            cout << " Your new request " << request.requestID << " waits behind it. For this demo we handle YOUR request now\n"
                 << " (Service Requests > Process next request always follows the rule).\n";
    }
    if (!requests.processRequestById(request.requestID, err))
    {
        cout << " [Error] " << err << "\n Workflow stopped.\n";
        return;
    }
    cout << " Task 2: request " << request.requestID << " is now Processing.\n";
    if (!demoContinue())
        return;

    // ---------------- Step 6: assign a resource (Task 3) ----------------
    demoPrintStep(6, "A collection counter or delivery resource is assigned");
    string resourceId = resources.assignNext(resourceType[route], request.requestID, err);
    if (resourceId.empty())
    {
        cout << " [Error] " << err << "\n"
             << " The request stays Processing. Free or add a resource in menu 3, then finish it in\n"
             << " Service Requests > Complete or return a processing request.\n Workflow stopped.\n";
        return;
    }
    cout << " Task 3: " << request.requestID << " assigned to " << resourceType[route] << " " << resourceId
         << " (round-robin; now Occupied).\n";
    if (!demoContinue())
        return;

    // ---------------- Step 7: process ----------------
    demoPrintStep(7, "The parcel / service is processed");
    cout << " " << resourceId << " now performs: " << serviceType[route] << " for " << parcel.recipientName
         << " (parcel " << parcel.parcelID << ").\n";
    if (!readYesNo(" Mark the service as performed?"))
    {
        cout << " Workflow stopped. The request stays Processing and " << resourceId << " stays Occupied.\n";
        return;
    }

    // ---------------- Step 8: update statuses ----------------
    demoPrintStep(8, "Parcel, request and resource status are updated");
    const string finalRequestStatus = (route == 3) ? "Returned" : "Completed";
    if (!requests.finishProcessingRequest(request.requestID, finalRequestStatus, err))
    {
        cout << " [Error] " << err << "\n Workflow stopped.\n";
        return;
    }
    cout << " Task 2: request " << request.requestID << " -> " << finalRequestStatus << ".\n";
    if (resources.release(resourceId, err))
        cout << " Task 3: " << resourceId << " released and Available again (transaction counted).\n";
    else
        cout << " [Error] " << err << "\n";
    if (!demoContinue())
        return;

    // ---------------- Step 9: history (Task 4) ----------------
    demoPrintStep(9, "The activity is recorded in system history");
    cout << " Task 4: the newest activities (newest first). Undo is available in menu 4.\n";
    getActivityManager().displayRecent(8);
    if (!demoContinue())
        return;

    // ---------------- Step 10: final status ----------------
    demoPrintStep(10, "Final status");
    Parcel *finalParcel = parcels.findParcel(parcel.parcelID);
    if (finalParcel != nullptr)
        ParcelManager::displayDetails(*finalParcel);
    const ServiceRequest *finalRequest = requests.findRequest(request.requestID);
    if (finalRequest != nullptr)
        cout << " Request " << finalRequest->requestID << " : " << finalRequest->status << "\n";
    Resource *finalResource = resources.find(resourceId);
    if (finalResource != nullptr)
        cout << " Resource " << finalResource->id << " : " << finalResource->status << " ("
             << finalResource->transactions << " transaction(s) handled)\n";
    Parcel *inTree = tree.searchParcel(parcel.parcelID);
    if (inTree != nullptr)
        cout << " Parcel tree : " << inTree->parcelID << " stored with status '" << inTree->status << "'\n";
    cout << "\n Workflow complete.\n";
}

// ---------------------------------------------------------------------
//  Main menu
// ---------------------------------------------------------------------
void showMainMenu()
{
    cout << "\n==============================================\n";
    cout << " APU SMART CAMPUS PARCEL MANAGEMENT SYSTEM\n";
    cout << "==============================================\n";
    cout << " 1. Parcel Intake & Sorting\n";
    cout << " 2. Service Requests\n";
    cout << " 3. Counters & Delivery Resources\n";
    cout << " 4. Activity History & Undo\n";
    cout << " 5. Parcel Category Tree\n";
    cout << " 6. Run End-to-End Demo Workflow\n";
    cout << " 0. Exit\n";
    cout << "----------------------------------------------\n";
}

int main()
{
    // Connect the five modules once, before any menu runs (see MODULE WIRING above).
    integrationConnect();

    int choice = -1;
    do
    {
        showMainMenu();
        choice = readInt(" Enter choice: ", 0, 6);

        // Input stream closed (Ctrl+Z / Ctrl+D / end of piped input):
        // leave the program instead of looping forever.
        if (cin.eof())
        {
            choice = 0;
            break;
        }

        switch (choice)
        {
        case 1:
            parcelMenu();
            break;
        case 2:
            requestMenu();
            break;
        case 3:
            resourceMenu();
            break;
        case 4:
            activityMenu();
            break;
        case 5:
            treeMenu();
            break;
        case 6:
            runWorkflowDemo();
            break;
        case 0:
            if (!readYesNo("\nAre you sure you want to exit?"))
            {
                choice = -1; // stay in the loop
            }
            break;
        }

        if (choice != 0 && choice != -1)
        {
            pauseScreen();
        }
    } while (choice != 0);

    cout << "\nThank you for using the system. Goodbye!\n";
    return 0;
}

// Include the implemented modules here so `g++ main.cpp -o main.exe`
// builds the complete system. Do not compile these files separately
// alongside main.cpp, or their definitions will be duplicated.
#include "common/Utils.cpp"
#include "task1_parcel_intake/parcelQueue.cpp"
#include "task1_parcel_intake/parcelManager.cpp"
#include "task2_service_reuqests/requestManager.cpp"
#include "task2_service_reuqests/requestPriorityQueue.cpp"
#include "task3_resources/resourceCircularQueue.cpp"
#include "task3_resources/resourceManager.cpp"
#include "task4_activity_history/activityStack.cpp"
#include "task4_activity_history/activityManager.cpp"
#include "task5_parcel_tree/parcelTree.cpp"
