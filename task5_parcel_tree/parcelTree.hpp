#ifndef PARCEL_TREE_HPP
#define PARCEL_TREE_HPP

#include <string>
#include "../common/Parcel.hpp"

// Node of the Binary Search Tree (key = Parcel ID)
struct ParcelNode {
    Parcel parcel;
    ParcelNode* left;
    ParcelNode* right;

    explicit ParcelNode(const Parcel& p) : parcel(p), left(nullptr), right(nullptr) {}
};

enum class InsertResult { INSERTED, DUPLICATE, INVALID };

class ParcelTree {
private:
    ParcelNode* root;
    int nodeCount;

    // Recursive helpers
    ParcelNode* insert(ParcelNode* node, const Parcel& parcel, bool& success);
    ParcelNode* search(ParcelNode* node, const std::string& parcelId) const;
    ParcelNode* deleteNode(ParcelNode* node, const std::string& parcelId, bool& success);
    ParcelNode* findMin(ParcelNode* node) const;
    void clear(ParcelNode* node);
    int  height(const ParcelNode* node) const;

    void inorder(const ParcelNode* node) const;
    void preorder(const ParcelNode* node) const;
    void postorder(const ParcelNode* node) const;
    void collectByCategory(const ParcelNode* node, const std::string& category, int& count) const;
    void printStructure(const ParcelNode* node, int depth) const;

public:
    ParcelTree();
    ~ParcelTree();
    ParcelTree(const ParcelTree&) = delete;            // owns raw pointers
    ParcelTree& operator=(const ParcelTree&) = delete;

    // ---- Core operations ----
    InsertResult insertParcel(const Parcel& parcel);
    Parcel*      searchParcel(const std::string& parcelId) const;
    int          searchByCategory(const std::string& category) const; // prints matches, returns count
    bool         updateParcel(const std::string& parcelId, const Parcel& newData); // key never changes
    bool         deleteParcel(const std::string& parcelId);

    // ---- Display ----
    void displayInOrder() const;
    void displayPreOrder() const;
    void displayPostOrder() const;
    void displayStructure() const;

    // ---- Info ----
    bool isEmpty() const;
    int  size() const;
    int  getHeight() const;

    // ---- Validation (public so the menu can reuse it) ----
    static bool isValidParcelId(const std::string& id);
    static bool isValidRecipientId(const std::string& id);
    static bool isValidDateTime(const std::string& dt);
    static bool isValidStatus(const std::string& status);
    static bool isValidCategory(const std::string& category);
    static bool validateParcel(const Parcel& p, std::string& error);
};

// Menu function interface for main.cpp
void treeMenu();

#endif