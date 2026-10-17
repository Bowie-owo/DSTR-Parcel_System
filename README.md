# APU Smart Campus Parcel Collection and Delivery Management System

A console application that manages parcels arriving at the campus delivery centre: registering and sorting parcels, handling service requests, allocating collection counters and delivery staff, recording activity history with undo, and organising parcel records in a category tree.

The system is split into five task modules plus a shared `common/` folder. `main.cpp` shows the main menu and hands control to each module.

---

---

## 1. Project structure

```
.
├── main.cpp                         main menu / navigation only
├── README.md
├── data/
│   ├── resources.txt                Task 3 data file (created on first save)
│   └── requests.txt                 sample service requests (Task 2 owner to confirm format)
├── common/                          shared by every task
│   ├── Parcel.hpp                   Parcel struct + status enum (no data structure here)
│   ├── ServiceRequest.hpp           ServiceRequest struct
│   ├── Resource.hpp                 Resource struct + resource types/statuses
│   ├── Activity.hpp                 Activity struct
│   ├── Utils.hpp / Utils.cpp        readInt, readYesNo, text and date/time helpers
├── task1_parcel_intake/
│   ├── parcelList.hpp/.cpp          linked list: stores all parcel records
│   ├── parcelQueue.hpp/.cpp         queue: parcels waiting to be processed
│   └── parcelManager.hpp/.cpp       uses both, plus parcelMenu()
├── task2_service_requests/
│   ├── requestPriorityQueue.hpp/.cpp   priority queue (sorted linked list or heap)
│   └── requestManager.hpp/.cpp         uses it, plus requestMenu()
├── task3_resources/
│   ├── resourceCircularQueue.hpp/.cpp  circular queue of counters/riders
│   └── resourceManager.hpp/.cpp        uses it, plus resourceMenu()
├── task4_activity_history/
│   ├── activityStack.hpp/.cpp          stack of activities (undo)
│   └── activityManager.hpp/.cpp        uses it, plus activityMenu()
└── task5_parcel_tree/
    └── parcelTree.hpp/.cpp             BST/AVL insert, search, delete, traversals, plus treeMenu()
```

> **Rule:** the assignment does not allow `vector`, `list`, `stack`, `queue`, `priority_queue`, `deque` (or other STL containers) as substitutes for the required data structures. Every structure above is hand-written.

---

## 2. Build and run

Build from the project root (the folder that contains `main.cpp`

---

## 3. Menus

### Main menu

```
==============================================
 APU SMART CAMPUS PARCEL MANAGEMENT SYSTEM
==============================================
 1. Parcel Intake & Sorting
 2. Service Requests
 3. Counters & Delivery Resources
 4. Activity History & Undo
 5. Parcel Category Tree
 6. Run End-to-End Demo Workflow
 0. Exit
```

### Task 1 – Parcel Intake & Sorting

```
--- PARCEL INTAKE & SORTING ---
 1. Register new parcel
 2. Display all parcels
 3. Search parcel by ID
 4. View waiting queue
 5. Process next waiting parcel
 6. Update parcel
 7. Remove/cancel parcel
 8. Report parcel missing
 0. Back
```

### Task 2 – Service Requests

```
--- SERVICE REQUESTS ---
 1. Create service request
 2. Display active requests
 3. Search request by ID
 4. View next request to handle
 5. Process next request
 6. Cancel request
 0. Back
```

### Task 3 – Resources

```
--- RESOURCES ---
 1. Register resource
 2. Display all resources
 3. Display availability
 4. Assign parcel/request
 5. Release resource
 6. Mark available/unavailable
 7. Remove resource
 0. Back
```

### Task 4 – Activity History

```
--- ACTIVITY HISTORY ---
 1. Show recent activities
 2. Show history for a parcel
 3. View most recent activity
 4. Undo last activity
 0. Back
```

### Task 5 – Parcel Tree

```
--- PARCEL TREE ---
 1. Insert parcel record
 2. Search parcel/category
 3. Display (in/pre/post-order)
 4. Update record
 5. Delete record
 0. Back
```

---

## 4. Modules and data structures

| Task | Module | Menu function | Data structure(s) | Owner |
|---|---|---|---|---|
| 1 | Parcel intake, sorting and processing | `parcelMenu()` | Linked list (all parcels) + queue (waiting parcels) | JiaHuey |
| 2 | Parcel service and collection management | `requestMenu()` | Priority queue (next request to handle) | ZhiLin |
| 3 | Collection counter and delivery resources | `resourceMenu()` | Circular queue (round-robin allocation) + plain array (records) | MingCyn |
| 4 | Activity history and undo | `activityMenu()` | Stack (last in, first out) | Bowie |
| 5 | Parcel category tree | `treeMenu()` | Binary search tree / AVL | SinYi |

---
