#ifndef BP_TREE_H
#define BP_TREE_H

#include <vector>
#include "record.h"

const int KEY_SIZE = 100;

int ceil_div(int a, int b);
int floor_div(int a, int b);

struct Node
{
    Node(int degree, bool is_leaf);
    ~Node();

    void insert(float key, Record *record);
    void split_child(int index);

    bool is_leaf;
    int degree;
    int size; // current number of keys
    float *keys;
    union
    {
        Node **node_values;
        Record **record_values;
    };
    Node *next;
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

private:
    int degree;
    int height;
    int size;
    Node *root;
};

#endif // BP_TREE_H