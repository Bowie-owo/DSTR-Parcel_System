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
//  Build (from project root):
//    g++ -std=c++17 -I. main.cpp common/*.cpp task*/*.cpp -o campus
//
//  While modules are still being written, build with the placeholder
//  menus below (default). Once a member's menu is ready, remove the
//  matching placeholder block and include their header.
// =====================================================================

#include <cstdlib>   // system()
#include <iostream>
#include <string>

// ---- Module headers: uncomment each one when that member is ready ----
// #include "task1_parcel_intake/parcelManager.hpp"
// #include "task2_service_reuqests/requestManager.hpp"
// #include "task3_resources/resourceManager.hpp"
// #include "task4_activity_history/activityManager.hpp"
// #include "task5_parcel_tree/parcelTree.hpp"

using namespace std;

// ---------------------------------------------------------------------
//  Menu entry points. Each owner implements their own in their folder:
//    Task 1 -> void parcelMenu();      (parcelManager.cpp)
//    Task 2 -> void requestMenu();     (requestManager.cpp)
//    Task 3 -> void resourceMenu();    (resourceManager.cpp)
//    Task 4 -> void activityMenu();    (activityManager.cpp)
//    Task 5 -> void treeMenu();        (parcelTree.cpp)
// ---------------------------------------------------------------------
#ifndef MODULES_READY
// ---- Temporary placeholders so main.cpp compiles on its own ----
void parcelMenu()   { cout << "\n[Task 1] Parcel Intake menu - not implemented yet.\n"; }
void requestMenu()  { cout << "\n[Task 2] Service Request menu - not implemented yet.\n"; }
void resourceMenu() { cout << "\n[Task 3] Resource menu - not implemented yet.\n"; }
void activityMenu() { cout << "\n[Task 4] Activity History menu - not implemented yet.\n"; }
void treeMenu()     { cout << "\n[Task 5] Parcel Tree menu - not implemented yet.\n"; }
#else
void parcelMenu();
void requestMenu();
void resourceMenu();
void activityMenu();
void treeMenu();
#endif

// ---------------------------------------------------------------------
//  Input helpers (move these into common/Utils later so all members
//  share the same validation)
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

// Reads an integer between minValue and maxValue (inclusive).
// Rejects empty input, letters, symbols, decimals and out-of-range values.
int readInt(const string &prompt, int minValue, int maxValue)
{
    while (true)
    {
        cout << prompt;
        string line;
        if (!getline(cin, line))
        {
            // Input stream closed (e.g. Ctrl+Z / Ctrl+D): exit safely.
            return minValue;
        }

        // Trim leading/trailing spaces
        size_t first = line.find_first_not_of(" \t");
        size_t last = line.find_last_not_of(" \t");
        if (first == string::npos)
        {
            cout << "  Invalid input: please enter a number.\n";
            continue;
        }
        line = line.substr(first, last - first + 1);

        // Must be digits only and a sensible length
        bool allDigits = true;
        for (size_t i = 0; i < line.size(); i++)
        {
            if (line[i] < '0' || line[i] > '9')
            {
                allDigits = false;
                break;
            }
        }
        if (!allDigits || line.size() > 9)
        {
            cout << "  Invalid input: digits only, please try again.\n";
            continue;
        }

        int value = 0;
        for (size_t i = 0; i < line.size(); i++)
        {
            value = value * 10 + (line[i] - '0');
        }

        if (value < minValue || value > maxValue)
        {
            cout << "  Out of range: choose between " << minValue
                 << " and " << maxValue << ".\n";
            continue;
        }
        return value;
    }
}

// Asks a Y/N question. Returns true for Y/y, false for N/n.
bool readYesNo(const string &prompt)
{
    while (true)
    {
        cout << prompt << " (Y/N): ";
        string line;
        if (!getline(cin, line))
        {
            return false;
        }
        if (line == "Y" || line == "y")
            return true;
        if (line == "N" || line == "n")
            return false;
        cout << "  Please enter Y or N.\n";
    }
}

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

        if (i < 9 && !readYesNo("Continue to next step?"))
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
}