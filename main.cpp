// =====================================================================
//  APU SMART CAMPUS PARCEL COLLECTION AND DELIVERY MANAGEMENT SYSTEM
//  main.cpp - Main menu / navigation interface
//
//  Each task menu function is implemented by the member who owns
//  that task (inside their own folder). main.cpp only:
//    1. shows the main menu
//    2. validates the user's choice
//    3. hands control to the correct module
//
//  Build from the project root in PowerShell:
//    g++ main.cpp -o main.exe
//  Build from the project root in PowerShell:
//    g++ main.cpp -o main.exe
//
//  Task 1 and 5 still use placeholder menus below. The implemented
//  Task 2, 3, and 4 source files are included at the end of this file so
//  the one-file build command above links their menus too.
// =====================================================================

#include <cstdlib>   // system()
#include <iostream>
#include <string>

#include "common/Utils.hpp" // shared input helpers: readInt, readYesNo, ...

// ---- Module headers: uncomment each one when that member is ready ----
// #include "task1_parcel_intake/parcelManager.hpp"
#include "task2_service_reuqests/requestManager.hpp"
#include "task3_resources/resourceManager.hpp"
#include "task4_activity_history/activityManager.hpp"
// #include "task5_parcel_tree/parcelTree.hpp"

using namespace std;
using namespace common;

// ---------------------------------------------------------------------
//  Menu entry points. Each owner implements their own in their folder:
//    Task 1 -> void parcelMenu();      (parcelManager.cpp)
//    Task 2 -> void requestMenu();     (requestManager.cpp)
//    Task 3 -> void resourceMenu();    (resourceManager.cpp)
//    Task 4 -> void activityMenu();    (activityManager.cpp)
//    Task 5 -> void treeMenu();        (parcelTree.cpp)
// ---------------------------------------------------------------------
#ifndef MODULES_READY
// ---- Placeholders for modules that are not implemented yet ----
// ---- Placeholders for modules that are not implemented yet ----
void parcelMenu()   { cout << "\n[Task 1] Parcel Intake menu - not implemented yet.\n"; }
// Task 2 and Task 3 provide their menu functions in their source files.
// void resourceMenu() { cout << "\n[Task 3] Resource menu - not implemented yet.\n"; }
// void activityMenu() { cout << "\n[Task 4] Activity History menu - not implemented yet.\n"; }
void treeMenu()     { cout << "\n[Task 5] Parcel Tree menu - not implemented yet.\n"; }
#else
void parcelMenu();
void activityMenu();
void treeMenu();
#endif

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

// readInt() and readYesNo() now come from common/Utils (namespace common),
// so every module validates input in exactly the same way.

// ---------------------------------------------------------------------
//  End-to-end workflow (integration demo)
//  Walks through the 10 steps in section 4 of the brief.
//  Fill in the calls to each module as they become available.
// ---------------------------------------------------------------------
void runWorkflowDemo()
{
    cout << "\n==============================================\n";
    cout << " END-TO-END WORKFLOW DEMO\n";
    cout << "==============================================\n";

    const char *steps[10] = {
        "Step 1  : Parcel arrives at the delivery centre",
        "Step 2  : Parcel is registered and information maintained",
        "Step 3  : Parcel is sorted / routed for its next action",
        "Step 4  : Service request is created",
        "Step 5  : Appropriate request is selected (priority rule)",
        "Step 6  : Collection counter / delivery resource assigned",
        "Step 7  : Parcel / service is processed",
        "Step 8  : Parcel, request and resource status updated",
        "Step 9  : Activity recorded in system history",
        "Step 10 : Final status shown (completed/returned/cancelled/hold)"};

    for (int i = 0; i < 10; i++)
    {
        cout << "\n" << steps[i] << "\n";

        // TODO: call the matching module function here, e.g.
        //   Step 2 -> parcels.registerParcel(...); activities.record(...);
        //   Step 4 -> requests.createRequest(...);
        //   Step 6 -> resources.assign(...);
        cout << "  (module call not connected yet)\n";

        if (i < 9 && !mainReadYesNo("Continue to next step?"))
        {
            cout << "Workflow stopped.\n";
            return;
        }
    }
    cout << "\nWorkflow complete.\n";
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
    // Task 4 <-> Task 3 connection: register the activity-history hook ONCE,
    // before any menu runs, so every resource action is recorded automatically.
    connectActivityLogging();

    // TODO (integration): create one object of each manager here and pass
    // references between them, for example:
    //   ParcelManager   parcels;
    //   ActivityManager activities;
    //   ResourceManager resources(parcels, activities);
    //   RequestManager  requests(parcels, resources, activities);
    //   ParcelTree      tree(parcels);
    // Then pass them into the menu functions.

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

// Include the implemented modules here so `g++ main.cpp -o main.exe`
// builds the complete current system. Do not compile these files
// separately alongside main.cpp, or their definitions will be duplicated.
#include "common/Utils.cpp"
#include "task2_service_reuqests/requestManager.cpp"
#include "task2_service_reuqests/requestPriorityQueue.cpp"
#include "task3_resources/resourceCircularQueue.cpp"
#include "task3_resources/resourceManager.cpp"
#include "task4_activity_history/activityStack.cpp"
#include "task4_activity_history/activityManager.cpp"