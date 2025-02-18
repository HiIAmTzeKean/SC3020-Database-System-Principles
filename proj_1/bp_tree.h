#ifndef BP_TREE_H
#define BP_TREE_H

#include <vector>
#include <utility>
#include "storage/storage.h"

const int KEY_SIZE = 3;

int ceil_div(int a, int b);
int floor_div(int a, int b);

struct Node
{
    Node(int degree, bool is_leaf);
    ~Node();

    Node* insert(float key, Record *record);
    Node* split_leaf_child(float key);
    Node* split_internal_child(float key);
    Node* split_child(int index, float key);

    bool is_leaf = 0;
    int degree = 0;
    int size = 0; // current number of keys
    float *keys = nullptr;

    union
    {
        Node **node_values;
        Record **record_values;
    };

    Node *next = nullptr;

    int get_child_degree() {
        if (is_leaf) {
            return degree;
        }
        return degree + 1;
    };
};

class BPlusTree
{
public:
    BPlusTree(int degree);
    ~BPlusTree();

    class Iterator
    {
    public:
        Iterator(Node *node, int index, float right_key);

        Record *operator*() const;
        Iterator &operator++();
        bool operator!=(const Iterator &other) const;

    private:
        Node *current;
        int index;
        float right_key;
    };

    Iterator search_range_begin(float left_key, float right_key);
    Iterator search_range_end();
    std::pair<Iterator, Iterator> search_range_iter(float left_key, float right_key);

    int get_index(float key, Node *node);
    Node *search_leaf_node(float key);
    Record *search(float key);
    std::vector<Record *> search_range_vector(float left_key, float right_key);
    void insert(float key, Record *value);
    void print() const;

private:
    int degree = 0;
    int height = 0;
    int size = 0;
    Node *root = nullptr;
};

#endif // BP_TREE_H