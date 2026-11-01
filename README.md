# APU Smart Campus Parcel Collection and Delivery Management System

A console application (C++) that manages parcels arriving at the campus delivery centre: registering and sorting parcels, handling service requests, allocating collection counters and delivery staff, recording activity history with undo, and organising parcel records in a category tree.

The system is split into five task modules plus a shared `common/` folder. `main.cpp` shows the main menu, **connects the modules to each other**, and runs the end-to-end workflow demo.

---

## 1. Project structure

```
.
├── main.cpp                         main menu, module wiring, end-to-end workflow demo
├── README.md
├── data/
│   ├── parcels.txt                  parcel records            (Task 1)
│   ├── requests.txt                 service request records   (Task 2)
│   ├── resources.txt                counters / riders         (Task 3)
│   └── activities.txt               activity history          (Task 4, created on first save)
├── common/                          shared by every task
│   ├── Parcel.hpp                   Parcel struct + status enum (no data structure here)
│   ├── serviceRequest.hpp           ServiceRequest struct
│   ├── Resource.hpp                 Resource struct + resource types/statuses
│   └── Utils.hpp / Utils.cpp        readInt, readYesNo, text and date/time helpers, ActivityLogger hook type
├── task1_parcel_intake/
│   ├── parcelQueue.hpp/.cpp         linked list (all parcels) + linked FIFO queue (waiting parcels)
│   └── parcelManager.hpp/.cpp       uses both, plus parcelManagerMenu()
├── task2_service_reuqests/
│   ├── requestPriorityQueue.hpp/.cpp   hand-written priority queue (sorted linked list)
│   └── requestManager.hpp/.cpp         uses it, plus requestMenu()
├── task3_resources/
│   ├── resourceCircularQueue.hpp/.cpp  circular queue of counters/riders
│   └── resourceManager.hpp/.cpp        uses it, plus resourceMenu()
├── task4_activity_history/
│   ├── activityStack.hpp/.cpp          linked stack of activities (newest on top)
│   └── activityManager.hpp/.cpp        uses it, plus activityMenu()
└── task5_parcel_tree/
    └── parcelTree.hpp/.cpp             binary search tree (key = Parcel ID), plus treeMenu()
```

> **Rule:** the assignment does not allow `vector`, `list`, `stack`, `queue`, `priority_queue`, `deque` (or other STL containers) as substitutes for the required data structures. Every structure above is hand-written.

---

## 2. Build and run

From the project root (the folder that contains `main.cpp`), in PowerShell:

```powershell
g++ -std=c++17 main.cpp -o main.exe
.\main.exe
```

All module `.cpp` files are `#include`d at the end of `main.cpp`, so compiling only `main.cpp` builds the complete system. Do **not** list those `.cpp` files again in the build command, or the linker will report duplicate definitions.

Run the program from the project root so the `data/` files are found.

---

## 3. How the modules work together

Every module reports its changes through a hook (`setActivityLogger`). `main.cpp` registers the hooks once at start-up (`integrationConnect`):

| From | To | What happens |
|---|---|---|
| Task 1 (parcels) | Task 4 | every register / route / status change / missing / cancel / remove is recorded in the history |
| Task 1 (parcels) | Task 5 | a registered parcel is inserted into the tree, status changes update it, removal deletes it |
| Task 2 (requests) | Task 4 | create / process / cancel / complete / return are recorded |
| Task 2 (requests) | Task 1 | completing or returning a request updates the associated parcel |
| Task 3 (resources) | Task 4 | register / status / assign / release / interrupt / remove are recorded |
| Task 4 (undo) | Task 1, 3 | undo restores the previous parcel or resource status (and the tree follows) |

Menu option **6 "Run End-to-End Demo Workflow"** walks one new parcel through the ten steps of the brief using the real modules.

---

## 4. Menus

### Main menu

```
 1. Parcel Intake & Sorting
 2. Service Requests
 3. Counters & Delivery Resources
 4. Activity History & Undo
 5. Parcel Category Tree
 6. Run End-to-End Demo Workflow
 0. Exit
```

### Task 1 - Parcel Intake & Sorting

```
 1. Register new parcel          6. Update parcel status
 2. Display all parcels          7. Report parcel missing
 3. Search / view parcel         8. Cancel parcel
 4. Display waiting queue        9. Remove parcel record
 5. Sort / process next parcel   0. Back to main menu
```

### Task 2 - Service Requests

```
 1. Create service request
 2. Display active requests
 3. Search request by ID
 4. View next request to handle
 5. Process next request
 6. Cancel request
 7. Complete or return a processing request
 8. Display completed/cancelled/returned requests
 9. Back
```

Priority rule for "next request": Special Handling, then Return to Sender, Re-delivery, Campus Delivery, Recipient Collection. Equal priority is ordered by oldest request time, then Request ID. A request moves Pending -> Processing before it can be completed, returned or cancelled. New requests are accepted only for a Parcel ID that exists in `data/parcels.txt`.

### Task 3 - Resources

```
 1. Register resource            5. Release resource
 2. Display all resources        6. Mark available/unavailable
 3. Display availability         7. Remove resource
 4. Assign parcel/request        0. Back
```

### Task 4 - Activity History

```
 1. Show recent activities
 2. Show history for a parcel/request/resource ID
 3. View most recent activity
 4. Undo last activity
 5. Undo a selected activity (by number)
 0. Back
```

Undo is last-in-first-out: only the newest activity that has not been reversed can be undone. Reversible: resource register/status, parcel register/route/status/missing/cancel. Other activities are reported as "cannot be reversed".

### Task 5 - Parcel Tree

```
 1. Insert parcel record (manual or import from parcels.txt)
 2. Search parcel / category
 3. Display (in-order / pre-order / post-order / structure diagram)
 4. Update record
 5. Delete record
 0. Back
```

---

## 5. Modules and data structures

| Task | Module | Menu function | Data structure(s) | Owner |
|---|---|---|---|---|
| 1 | Parcel intake, sorting and processing | `parcelManagerMenu()` | Linked list (all parcels) + queue (waiting parcels) | JiaHuey |
| 2 | Parcel service and collection management | `requestMenu()` | Priority queue (next request to handle) + linked list (all requests) | ZhiLin |
| 3 | Collection counter and delivery resources | `resourceMenu()` | Circular queue (round-robin allocation) + plain array (records) | MingCyn |
| 4 | Activity history and undo | `activityMenu()` | Stack (last in, first out) | Bowie |
| 5 | Parcel category tree | `treeMenu()` | Binary search tree | SinYi |

---

## 6. Data files

| File | Format |
|---|---|
| `data/parcels.txt` | `ParcelID\|RecipientID\|RecipientName\|ParcelType\|ArrivalDateTime\|Status\|DeliveryInfo` |
| `data/requests.txt` | `RequestID\|ParcelID\|RecipientID\|RecipientName\|ServiceType\|DateTime\|Status` |
| `data/resources.txt` | `ResourceID\|Type\|Location\|Status\|CurrentItem\|Transactions` |
| `data/activities.txt` | tab-separated: number, time, action, item, resource, previous status, new status, reversed (0/1), reversed time, details |