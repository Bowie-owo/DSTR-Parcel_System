#include "parcelTree.hpp"
#include "../common/Utils.hpp"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>

// NOTE: no "using namespace common" here on purpose - helper names below are
// unique so they never clash when everything is built as one translation unit.

// =====================================================================
//  Local helpers & constants
// =====================================================================
namespace {

const char* tree_PARCEL_FILE = "data/parcels.txt";

const int tree_NUM_STATUSES = 11;
const char* tree_VALID_STATUSES[tree_NUM_STATUSES] = {
    "Pending", "Registered", "Sorted", "Processing", "Ready for Collection",
    "Out for Delivery", "Completed", "Returned", "Missing", "Cancelled", "Special Handling"};

const int tree_NUM_CATEGORIES = 5;
const char* tree_VALID_CATEGORIES[tree_NUM_CATEGORIES] = {
    "Standard Parcel", "Fragile Box", "Document", "Electronics", "Clothing"};

std::string tree_cleanText(const std::string& s) {          // trim whitespace / CR
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

std::string tree_upperCopy(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return std::toupper(c); });
    return s;
}

std::string tree_lowerCopy(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return s;
}

// Returns the canonical spelling from a list (case-insensitive), or "" if absent.
std::string tree_canonicalFrom(const char* list[], int size, const std::string& value) {
    for (int i = 0; i < size; ++i) {
        if (tree_lowerCopy(list[i]) == tree_lowerCopy(tree_cleanText(value))) return list[i];
    }
    return "";
}

// Reads a whole line (spaces allowed). Returns "" on end-of-input.
std::string tree_askText(const std::string& prompt) {
    std::cout << prompt;
    std::string line;
    if (!std::getline(std::cin, line)) return "";
    return tree_cleanText(line);
}

// Keeps asking until validator accepts. Returns "" only if input closed.
template <typename Validator>
std::string tree_askValidated(const std::string& prompt, Validator ok, const std::string& errorMsg) {
    while (true) {
        std::string v = tree_askText(prompt);
        if (std::cin.eof()) return "";
        if (ok(v)) return v;
        std::cout << "  [!] " << errorMsg << "\n";
    }
}

// Numbered pick-list. minChoice = 0 allows "keep current" on update.
std::string tree_chooseFromList(const std::string& title, const char* list[], int size, bool allowKeep) {
    std::cout << title << "\n";
    if (allowKeep) std::cout << "  0. (keep current)\n";
    for (int i = 0; i < size; ++i)
        std::cout << "  " << (i + 1) << ". " << list[i] << "\n";
    int c = common::readInt("  Select: ", allowKeep ? 0 : 1, size);
    return c == 0 ? "" : list[c - 1];
}

void tree_printBanner(const std::string& title) {
    std::cout << "\n+" << std::string(58, '=') << "+\n";
    int pad = static_cast<int>((58 - title.size()) / 2);
    std::cout << "|" << std::string(pad, ' ') << title
              << std::string(58 - pad - title.size(), ' ') << "|\n";
    std::cout << "+" << std::string(58, '=') << "+\n";
}

void tree_printTableHeader() {
    std::cout << "\n" << std::left
              << std::setw(8)  << "ID"
              << std::setw(10) << "Recip.ID"
              << std::setw(18) << "Recipient"
              << std::setw(17) << "Category"
              << std::setw(18) << "Arrived"
              << std::setw(12) << "Status"
              << "Location\n"
              << std::string(100, '-') << "\n";
}

void tree_printTableRow(const Parcel& p) {
    std::cout << std::left
              << std::setw(8)  << p.parcelID
              << std::setw(10) << p.recipientID
              << std::setw(18) << p.recipientName.substr(0, 17)
              << std::setw(17) << p.parcelType.substr(0, 16)
              << std::setw(18) << p.arrivalDateTime
              << std::setw(12) << p.status.substr(0, 11)
              << p.deliveryInfo << "\n";
}

std::string tree_normaliseId(const std::string& id) { return tree_upperCopy(tree_cleanText(id)); }

}  // namespace

// =====================================================================
//  Construction / destruction
// =====================================================================
ParcelTree::ParcelTree() : root(nullptr), nodeCount(0) {}

ParcelTree::~ParcelTree() { clear(root); }

void ParcelTree::clear(ParcelNode* node) {
    if (!node) return;
    clear(node->left);
    clear(node->right);
    delete node;
}

bool ParcelTree::isEmpty() const { return root == nullptr; }
int  ParcelTree::size() const { return nodeCount; }
int  ParcelTree::getHeight() const { return height(root); }

int ParcelTree::height(const ParcelNode* node) const {
    if (!node) return 0;
    return 1 + std::max(height(node->left), height(node->right));
}

// =====================================================================
//  Validation
// =====================================================================
bool ParcelTree::isValidParcelId(const std::string& id) {          // PCL + 3 digits
    if (id.size() != 6 || id.compare(0, 3, "PCL") != 0) return false;
    return std::all_of(id.begin() + 3, id.end(), [](unsigned char c) { return std::isdigit(c); });
}

bool ParcelTree::isValidRecipientId(const std::string& id) {        // TP + 6 digits
    if (id.size() != 8 || id.compare(0, 2, "TP") != 0) return false;
    return std::all_of(id.begin() + 2, id.end(), [](unsigned char c) { return std::isdigit(c); });
}

bool ParcelTree::isValidDateTime(const std::string& dt) {          // YYYY-MM-DD HH:MM
    if (dt.size() != 16) return false;
    for (int i = 0; i < 16; ++i) {
        bool sep = (i == 4 || i == 7) ? dt[i] == '-' :
                   (i == 10)         ? dt[i] == ' ' :
                   (i == 13)         ? dt[i] == ':' :
                                     std::isdigit(static_cast<unsigned char>(dt[i])) != 0;
        if (!sep) return false;
    }
    int month = std::stoi(dt.substr(5, 2)),  day = std::stoi(dt.substr(8, 2));
    int hour  = std::stoi(dt.substr(11, 2)), min = std::stoi(dt.substr(14, 2));
    return month >= 1 && month <= 12 && day >= 1 && day <= 31 && hour <= 23 && min <= 59;
}

bool ParcelTree::isValidStatus(const std::string& status) {
    return !tree_canonicalFrom(tree_VALID_STATUSES, tree_NUM_STATUSES, status).empty();
}

bool ParcelTree::isValidCategory(const std::string& category) {
    return !tree_canonicalFrom(tree_VALID_CATEGORIES, tree_NUM_CATEGORIES, category).empty();
}

bool ParcelTree::validateParcel(const Parcel& p, std::string& error) {
    if (!isValidParcelId(p.parcelID))         { error = "Invalid Parcel ID (expected PCL + 3 digits, e.g. PCL001)."; return false; }
    if (!isValidRecipientId(p.recipientID))   { error = "Invalid Recipient ID (expected TP + 6 digits, e.g. TP012345)."; return false; }
    if (tree_cleanText(p.recipientName).empty())   { error = "Recipient name cannot be empty."; return false; }
    if (!isValidCategory(p.parcelType))       { error = "Invalid parcel category '" + p.parcelType + "'."; return false; }
    if (!isValidDateTime(p.arrivalDateTime))  { error = "Invalid arrival date/time (expected YYYY-MM-DD HH:MM)."; return false; }
    if (!isValidStatus(p.status))             { error = "Invalid status '" + p.status + "'."; return false; }
    if (tree_cleanText(p.deliveryInfo).empty())    { error = "Location / delivery info cannot be empty."; return false; }
    return true;
}

// =====================================================================
//  Insert
// =====================================================================
ParcelNode* ParcelTree::insert(ParcelNode* node, const Parcel& parcel, bool& success) {
    if (!node) {
        success = true;
        return new ParcelNode(parcel);
    }
    if (parcel.parcelID < node->parcel.parcelID)
        node->left = insert(node->left, parcel, success);
    else if (parcel.parcelID > node->parcel.parcelID)
        node->right = insert(node->right, parcel, success);
    else
        success = false;                       // duplicate key
    return node;
}

InsertResult ParcelTree::insertParcel(const Parcel& input) {
    Parcel p = input;
    p.parcelID      = tree_normaliseId(p.parcelID);
    p.recipientID   = tree_upperCopy(tree_cleanText(p.recipientID));
    p.recipientName = tree_cleanText(p.recipientName);
    p.arrivalDateTime = tree_cleanText(p.arrivalDateTime);
    p.deliveryInfo  = tree_cleanText(p.deliveryInfo);
    
    std::string canonicalCat = tree_canonicalFrom(tree_VALID_CATEGORIES, tree_NUM_CATEGORIES, p.parcelType);
    p.parcelType    = canonicalCat.empty() ? p.parcelType : canonicalCat;

    std::string canonicalStatus = tree_canonicalFrom(tree_VALID_STATUSES, tree_NUM_STATUSES, p.status);
    p.status        = canonicalStatus.empty() ? p.status : canonicalStatus;

    std::string err;
    if (!validateParcel(p, err)) return InsertResult::INVALID;

    bool success = false;
    root = insert(root, p, success);
    if (!success) return InsertResult::DUPLICATE;
    ++nodeCount;
    return InsertResult::INSERTED;
}

// =====================================================================
//  Search
// =====================================================================
ParcelNode* ParcelTree::search(ParcelNode* node, const std::string& parcelId) const {
    if (!node || node->parcel.parcelID == parcelId) return node;
    return parcelId < node->parcel.parcelID ? search(node->left, parcelId)
                                            : search(node->right, parcelId);
}

Parcel* ParcelTree::searchParcel(const std::string& parcelId) const {
    ParcelNode* found = search(root, tree_normaliseId(parcelId));
    return found ? &(found->parcel) : nullptr;
}

void ParcelTree::collectByCategory(const ParcelNode* node, const std::string& category,
                                   int& count) const {
    if (!node) return;
    collectByCategory(node->left, category, count);
    if (tree_lowerCopy(node->parcel.parcelType) == tree_lowerCopy(category)) {
        if (count == 0) tree_printTableHeader();
        tree_printTableRow(node->parcel);
        ++count;
    }
    collectByCategory(node->right, category, count);
}

int ParcelTree::searchByCategory(const std::string& category) const {
    int count = 0;
    collectByCategory(root, tree_cleanText(category), count);
    return count;
}

// =====================================================================
//  Traversals / display
// =====================================================================
void ParcelTree::inorder(const ParcelNode* n) const {
    if (!n) return;
    inorder(n->left);
    tree_printTableRow(n->parcel);
    inorder(n->right);
}

void ParcelTree::preorder(const ParcelNode* n) const {
    if (!n) return;
    tree_printTableRow(n->parcel);
    preorder(n->left);
    preorder(n->right);
}

void ParcelTree::postorder(const ParcelNode* n) const {
    if (!n) return;
    postorder(n->left);
    postorder(n->right);
    tree_printTableRow(n->parcel);
}

void ParcelTree::displayInOrder() const {
    if (isEmpty()) { std::cout << "\n[!] The parcel tree is empty.\n"; return; }
    std::cout << "\n--- PARCEL RECORDS (IN-ORDER: Left > Root > Right, sorted by ID) ---";
    tree_printTableHeader();
    inorder(root);
}

void ParcelTree::displayPreOrder() const {
    if (isEmpty()) { std::cout << "\n[!] The parcel tree is empty.\n"; return; }
    std::cout << "\n--- PARCEL RECORDS (PRE-ORDER: Root > Left > Right) ---";
    tree_printTableHeader();
    preorder(root);
}

void ParcelTree::displayPostOrder() const {
    if (isEmpty()) { std::cout << "\n[!] The parcel tree is empty.\n"; return; }
    std::cout << "\n--- PARCEL RECORDS (POST-ORDER: Left > Right > Root) ---";
    tree_printTableHeader();
    postorder(root);
}

void ParcelTree::printStructure(const ParcelNode* n, int depth) const {
    if (!n) return;
    printStructure(n->right, depth + 1);
    std::cout << std::string(depth * 8, ' ') << n->parcel.parcelID << "\n";
    printStructure(n->left, depth + 1);
}

void ParcelTree::displayStructure() const {
    if (isEmpty()) { std::cout << "\n[!] The parcel tree is empty.\n"; return; }
    std::cout << "\n--- TREE STRUCTURE (rotated 90 degrees: right branch on top) ---\n";
    printStructure(root, 0);
    std::cout << "\nRoot: " << root->parcel.parcelID
              << " | Records: " << nodeCount
              << " | Height: " << getHeight() << "\n";
}

// =====================================================================
//  Update (the key / Parcel ID is never changed, so BST order is preserved)
// =====================================================================
bool ParcelTree::updateParcel(const std::string& parcelId, const Parcel& d) {
    ParcelNode* found = search(root, tree_normaliseId(parcelId));
    if (!found) return false;
    found->parcel.recipientName = d.recipientName;
    found->parcel.parcelType    = d.parcelType;
    found->parcel.status        = d.status;
    found->parcel.deliveryInfo  = d.deliveryInfo;
    return true;
}

// =====================================================================
//  Delete (handles leaf, one child, two children)
// =====================================================================
ParcelNode* ParcelTree::findMin(ParcelNode* node) const {
    while (node && node->left) node = node->left;
    return node;
}

ParcelNode* ParcelTree::deleteNode(ParcelNode* node, const std::string& parcelId, bool& success) {
    if (!node) { success = false; return nullptr; }

    if (parcelId < node->parcel.parcelID) {
        node->left = deleteNode(node->left, parcelId, success);
    } else if (parcelId > node->parcel.parcelID) {
        node->right = deleteNode(node->right, parcelId, success);
    } else {
        success = true;
        if (!node->left) {                     // leaf or only right child
            ParcelNode* child = node->right;
            delete node;
            return child;
        }
        if (!node->right) {                    // only left child
            ParcelNode* child = node->left;
            delete node;
            return child;
        }
        // Two children: replace with in-order successor, then delete the successor
        ParcelNode* succ = findMin(node->right);
        node->parcel = succ->parcel;
        bool dummy = false;
        node->right = deleteNode(node->right, succ->parcel.parcelID, dummy);
    }
    return node;
}

bool ParcelTree::deleteParcel(const std::string& parcelId) {
    bool success = false;
    root = deleteNode(root, tree_normaliseId(parcelId), success);
    if (success) --nodeCount;
    return success;
}

// =====================================================================
//  File import
// =====================================================================
// Imports one parcel (targetId) or all parcels (targetId empty) from parcels.txt
static void tree_importFromFile(ParcelTree& tree, const std::string& targetId) {
    std::ifstream file(tree_PARCEL_FILE);
    if (!file.is_open()) {
        std::cout << "[!] Could not open " << tree_PARCEL_FILE << "\n";
        return;
    }

    std::string line;
    int inserted = 0, duplicates = 0, invalid = 0;
    bool foundTarget = false;

    while (std::getline(file, line)) {
        line = tree_cleanText(line);
        if (line.empty()) continue;

        std::string fields[7];
        int fieldCount = 0;
        size_t start = 0, pos;
        while ((pos = line.find('|', start)) != std::string::npos && fieldCount < 7) {
            fields[fieldCount++] = tree_cleanText(line.substr(start, pos - start));
            start = pos + 1;
        }
        if (fieldCount < 7) {
            fields[fieldCount++] = tree_cleanText(line.substr(start));
        }

        if (fieldCount != 7) { ++invalid; continue; }   // malformed line

        if (!targetId.empty() && tree_upperCopy(fields[0]) != targetId) continue;
        foundTarget = true;

        Parcel p{fields[0], fields[1], fields[2], fields[3], fields[4], fields[5], fields[6]};
        switch (tree.insertParcel(p)) {
            case InsertResult::INSERTED:  ++inserted;   break;
            case InsertResult::DUPLICATE: ++duplicates; break;
            case InsertResult::INVALID:   ++invalid;    break;
        }
        if (!targetId.empty()) break;
    }

    if (!targetId.empty() && !foundTarget) {
        std::cout << "[!] Parcel ID '" << targetId << "' was not found in " << tree_PARCEL_FILE << ".\n";
        return;
    }
    std::cout << "Import finished: " << inserted << " inserted, "
              << duplicates << " duplicate(s) skipped, "
              << invalid << " invalid record(s) skipped.\n";
}

// =====================================================================
//  Menu handlers
// =====================================================================
static void tree_insertManually(ParcelTree& tree) {
    std::cout << "\n--- INSERT NEW PARCEL RECORD ---\n";
    Parcel p;

    p.parcelID = tree_upperCopy(tree_askValidated(
        "Parcel ID (e.g. PCL009)      : ",
        [](const std::string& s) { return ParcelTree::isValidParcelId(tree_upperCopy(s)); },
        "Invalid Parcel ID. Use PCL followed by 3 digits."));
    if (p.parcelID.empty()) return;

    if (tree.searchParcel(p.parcelID)) {
        std::cout << "[!] Duplicate Parcel ID: " << p.parcelID << " already exists in the tree.\n";
        return;
    }

    p.recipientID = tree_upperCopy(tree_askValidated(
        "Recipient ID (e.g. TP012345) : ",
        [](const std::string& s) { return ParcelTree::isValidRecipientId(tree_upperCopy(s)); },
        "Invalid Recipient ID. Use TP followed by 6 digits."));
    p.recipientName = tree_askValidated(
        "Recipient name               : ",
        [](const std::string& s) { return !s.empty(); },
        "Recipient name cannot be empty.");
    p.parcelType = tree_chooseFromList("Parcel category:", tree_VALID_CATEGORIES, tree_NUM_CATEGORIES, false);
    p.arrivalDateTime = tree_askValidated(
        "Arrival (YYYY-MM-DD HH:MM)   : ", ParcelTree::isValidDateTime,
        "Invalid date/time. Example: 2026-10-16 09:00");
    p.status = tree_chooseFromList("Status:", tree_VALID_STATUSES, tree_NUM_STATUSES, false);
    p.deliveryInfo = tree_askValidated(
        "Location / delivery info     : ",
        [](const std::string& s) { return !s.empty(); },
        "Location cannot be empty.");

    switch (tree.insertParcel(p)) {
        case InsertResult::INSERTED:  std::cout << "\n[OK] Parcel " << p.parcelID << " inserted.\n"; break;
        case InsertResult::DUPLICATE: std::cout << "\n[!] Duplicate Parcel ID.\n"; break;
        case InsertResult::INVALID: {
            std::string err; ParcelTree::validateParcel(p, err);
            std::cout << "\n[!] Invalid parcel information: " << err << "\n";
        }
    }
}

static void tree_insertMenu(ParcelTree& tree) {
    std::cout << "\n--- INSERT PARCEL RECORD ---\n"
              << "  1. Enter a new record manually\n"
              << "  2. Import ONE record from parcels.txt\n"
              << "  3. Import ALL records from parcels.txt\n"
              << "  0. Back\n";
    int c = common::readInt("Enter choice: ", 0, 3);
    if (c == 1) tree_insertManually(tree);
    else if (c == 2) {
        std::string id = tree_upperCopy(tree_askValidated(
            "Parcel ID to import (e.g. PCL001): ",
            [](const std::string& s) { return ParcelTree::isValidParcelId(tree_upperCopy(s)); },
            "Invalid Parcel ID. Use PCL followed by 3 digits."));
        if (!id.empty()) tree_importFromFile(tree, id);
    } else if (c == 3) tree_importFromFile(tree, "");
}

static void tree_searchMenu(ParcelTree& tree) {
    if (tree.isEmpty()) { std::cout << "\n[!] The parcel tree is empty. Nothing to search.\n"; return; }

    std::cout << "\n--- SEARCH ---\n"
              << "  1. Search by Parcel ID (key)\n"
              << "  2. Search by Category\n"
              << "  0. Back\n";
    int c = common::readInt("Enter choice: ", 0, 2);

    if (c == 1) {
        std::string id = tree_upperCopy(tree_askValidated(
            "Parcel ID to search: ",
            [](const std::string& s) { return ParcelTree::isValidParcelId(tree_upperCopy(s)); },
            "Invalid Parcel ID. Use PCL followed by 3 digits."));
        if (id.empty()) return;
        Parcel* p = tree.searchParcel(id);
        if (p) { std::cout << "\n[OK] Parcel found:"; tree_printTableHeader(); tree_printTableRow(*p); }
        else   std::cout << "\n[!] Parcel " << id << " does not exist in the tree.\n";
    } else if (c == 2) {
        std::string cat = tree_chooseFromList("Select category to search:", tree_VALID_CATEGORIES, tree_NUM_CATEGORIES, false);
        int n = tree.searchByCategory(cat);
        if (n == 0) std::cout << "\n[!] No parcels found in category '" << cat << "'.\n";
        else        std::cout << "\n[OK] " << n << " parcel(s) found in category '" << cat << "'.\n";
    }
}

static void tree_displayMenu(ParcelTree& tree) {
    if (tree.isEmpty()) { std::cout << "\n[!] The parcel tree is empty.\n"; return; }
    std::cout << "\n--- DISPLAY PARCEL RECORDS ---\n"
              << "  1. In-Order   (sorted by Parcel ID)\n"
              << "  2. Pre-Order\n"
              << "  3. Post-Order\n"
              << "  4. Tree structure diagram\n"
              << "  0. Back\n";
    int c = common::readInt("Enter choice: ", 0, 4);
    if (c == 1) tree.displayInOrder();
    else if (c == 2) tree.displayPreOrder();
    else if (c == 3) tree.displayPostOrder();
    else if (c == 4) tree.displayStructure();
}

static void tree_updateRecord(ParcelTree& tree) {
    if (tree.isEmpty()) { std::cout << "\n[!] The parcel tree is empty. Nothing to update.\n"; return; }

    std::string id = tree_upperCopy(tree_askValidated(
        "Parcel ID to update: ",
        [](const std::string& s) { return ParcelTree::isValidParcelId(tree_upperCopy(s)); },
        "Invalid Parcel ID. Use PCL followed by 3 digits."));
    if (id.empty()) return;

    Parcel* cur = tree.searchParcel(id);
    if (!cur) { std::cout << "\n[!] Cannot update: Parcel " << id << " does not exist.\n"; return; }

    std::cout << "\nCurrent record:"; tree_printTableHeader(); tree_printTableRow(*cur);
    std::cout << "\n(Parcel ID is the tree key and cannot be changed. Leave blank / choose 0 to keep a value.)\n";

    Parcel updated = *cur;

    std::string name = tree_askText("New recipient name [" + cur->recipientName + "]: ");
    if (!name.empty()) updated.recipientName = name;

    std::string cat = tree_chooseFromList("New category:", tree_VALID_CATEGORIES, tree_NUM_CATEGORIES, true);
    if (!cat.empty()) updated.parcelType = cat;

    std::string st = tree_chooseFromList("New status:", tree_VALID_STATUSES, tree_NUM_STATUSES, true);
    if (!st.empty()) updated.status = st;

    std::string loc = tree_askText("New location/info [" + cur->deliveryInfo + "]: ");
    if (!loc.empty()) updated.deliveryInfo = loc;

    if (tree.updateParcel(id, updated)) {
        std::cout << "\n[OK] Parcel " << id << " updated:";
        tree_printTableHeader(); tree_printTableRow(*tree.searchParcel(id));
    } else {
        std::cout << "\n[!] Update failed.\n";
    }
}

static void tree_deleteRecord(ParcelTree& tree) {
    if (tree.isEmpty()) { std::cout << "\n[!] The parcel tree is empty. Nothing to delete.\n"; return; }

    std::string id = tree_upperCopy(tree_askValidated(
        "Parcel ID to delete: ",
        [](const std::string& s) { return ParcelTree::isValidParcelId(tree_upperCopy(s)); },
        "Invalid Parcel ID. Use PCL followed by 3 digits."));
    if (id.empty()) return;

    Parcel* p = tree.searchParcel(id);
    if (!p) { std::cout << "\n[!] Cannot delete: Parcel " << id << " does not exist.\n"; return; }

    std::cout << "\nRecord to delete:"; tree_printTableHeader(); tree_printTableRow(*p);
    if (!common::readYesNo("\nAre you sure you want to delete this record?")) {
        std::cout << "Deletion cancelled.\n";
        return;
    }
    if (tree.deleteParcel(id)) {
        std::cout << "\n[OK] Parcel " << id << " deleted. Tree re-organised (BST property kept).\n";
        if (tree.isEmpty()) std::cout << "[i] The tree is now empty.\n";
    }
}

// =====================================================================
//  Main module menu (called from main.cpp)
// =====================================================================
void treeMenu() {
    static ParcelTree tree;     // static: records survive leaving and re-entering the menu
    int choice = -1;

    do {
        tree_printBanner("PARCEL CATEGORY ORGANISATION (BST)");
        std::cout << "  Records: " << tree.size() << "   Tree height: " << tree.getHeight() << "\n"
                  << "  " << std::string(40, '-') << "\n"
                  << "  1. Insert parcel record\n"
                  << "  2. Search parcel / category\n"
                  << "  3. Display parcel records\n"
                  << "  4. Update parcel record\n"
                  << "  5. Delete parcel record\n"
                  << "  0. Back to main menu\n"
                  << "  " << std::string(40, '-') << "\n";

        choice = common::readInt("  Enter choice: ", 0, 5);
        if (std::cin.eof()) break;

        switch (choice) {
            case 1: tree_insertMenu(tree);   break;
            case 2: tree_searchMenu(tree);   break;
            case 3: tree_displayMenu(tree);  break;
            case 4: tree_updateRecord(tree); break;
            case 5: tree_deleteRecord(tree); break;
            default: break;
        }
    } while (choice != 0);
}