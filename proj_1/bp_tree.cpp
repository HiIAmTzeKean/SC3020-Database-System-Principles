#include "record.h"
#include "bp_tree.h"
#include <vector>
#include <iostream>


int ceil_div(int a, int b)
{
    return (a + b - 1) / b;
};

int floor_div(int a, int b)
{
    return a / b;
};

Node::Node(int degree, bool is_leaf) : is_leaf(is_leaf), degree(degree)
{
    this->keys = new float[degree];
    if (this->is_leaf)
    {
        this->record_values = new Record *[degree];
        for (int i = 0; i < degree; i++)
        {
            record_values[i] = nullptr;
        }
        this->next = nullptr;
    }
    else
    {   
        // internal node has degree + 1 pointers
        degree++;
        this->degree = degree;
        this->node_values = new Node *[degree];
        for (int i = 0; i < degree; i++)
        {
            node_values[i] = nullptr;
        }
    }
};

Node::~Node()
{
    delete[] keys;
    if (is_leaf)
    {
        delete[] record_values;
    }
    else
    {
        delete[] node_values;
    }
};

void Node::insert(float key, Record *record)
{
    int i = this->size - 1;
    if (this->is_leaf)
    {
        std::cout << "Inserting into leaf node." << std::endl;
        while (i >= 0 && this->keys[i] > key)
        {
            this->keys[i + 1] = this->keys[i];
            record_values[i + 1] = record_values[i];
            i--;
        }
        this->keys[i + 1] = key;
        this->record_values[i + 1] = record;
        this->size++;
    }
    else
    {
        std::cout << "Inserting into internal node." << std::endl;
        // insert into child node
        while (i >= 0 && this->keys[i] > key)
        {
            i--;
        }
        std::cout << "Child node index: " << i + 1 << std::endl;
        std::cout << "Child node size: " << node_values[i + 1]->size << std::endl;
        // check if child node is full
        if (node_values[i + 1]->size == degree)
        {
            std::cout << "Child node is full. Splitting child node." << std::endl;
            split_child(i + 1);
            if (keys[i + 1] < key)
            {
                i++;
            }
        }
        node_values[i + 1]->insert(key, record);
    }
};

void Node::split_child(int index)
{
    Node *child = node_values[index];
    Node *new_child = new Node(this->degree, child->is_leaf);
    int t = 0;
    if (child->is_leaf)
    {
        std::cout << "Splitting leaf node." << std::endl;
        t = (this->degree + 1) / 2;
    }
    else
    {
        std::cout << "Splitting internal node." << std::endl;
        t = this->degree / 2 + 1;
    }
    child->size = this->degree - t;
    new_child->size = t;

    // new child holds the second half of the keys
    if (child->is_leaf)
    {
        for (int i = 0; i < t; i++)
        {
            new_child->keys[i] = child->keys[i + t];
            new_child->record_values[i] = child->record_values[i + t];
            child->record_values[i + t] = nullptr;
        }
        new_child->next = child->next;
        child->next = new_child;
    }
    else
    {
        for (int i = 0; i < t; i++)
        {
            new_child->keys[i] = child->keys[i + t];
            new_child->node_values[i] = child->node_values[i + t];
            child->node_values[i + t] = nullptr;
        }
    }

    // update node key and values
    for (int i = this->size; i > index; i--)
    {
        this->keys[i] = this->keys[i - 1];
        this->node_values[i + 1] = this->node_values[i];
    }
    this->keys[index] = child->keys[t - 1];
    this->node_values[index + 1] = new_child;
    this->size++;
};

BPlusTree::BPlusTree(int degree)
{
    this->degree = degree;
    this->root = new Node(degree, 1);
};

BPlusTree::~BPlusTree()
{
    delete root;
};

BPlusTree::Iterator::Iterator(Node *node, int index, float right_key)
    : current(node), index(index), right_key(right_key) {}

Record *BPlusTree::Iterator::operator*() const
{
    return current->record_values[index];
};

BPlusTree::Iterator &BPlusTree::Iterator::operator++()
{
    if (current == nullptr)
    {
        return *this;
    }
    index++;
    if (index >= current->size || current->keys[index] > right_key)
    {
        current = current->next;
        index = 0;
    }
    return *this;
};

bool BPlusTree::Iterator::operator!=(const Iterator &other) const
{
    return current != other.current || index != other.index;
};

BPlusTree::Iterator BPlusTree::search_range_begin(float left_key, float right_key)
{
    Node *current = search_leaf_node(left_key);
    if (current == nullptr)
    {
        return Iterator(nullptr, 0, right_key);
    }
    int i = 0;
    while (i < current->size && current->keys[i] < left_key)
    {
        i++;
    }
    return Iterator(current, i, right_key);
};

BPlusTree::Iterator BPlusTree::search_range_end()
{
    return Iterator(nullptr, 0, 0);
};

int BPlusTree::get_index(float key, Node *node)
{
    for (int i = 0; i < node->size && node->keys[i] <= key; i++)
    {
        if (node->keys[i] == key)
        {
            return i;
        }
    }
    return -1;
};

Node *BPlusTree::search_leaf_node(float key)
{
    if (root == nullptr)
    {
        return nullptr;
    }
    Node *current = root;
    while (!current->is_leaf)
    {
        int i = 0;
        while (i < current->size && current->keys[i] < key)
        {
            i++;
        }
        current = current->node_values[i];
    }
    return current;
};

Record *BPlusTree::search(float key)
{
    Node *current = search_leaf_node(key);
    if (current == nullptr)
    {
        return nullptr;
    }
    // current is a leaf node
    int index = get_index(key, current);
    if (index == -1)
    {
        return nullptr;
    }
    return current->record_values[index];
};

std::vector<Record *> BPlusTree::search_range_vector(float left_key, float right_key)
{
    std::vector<Record *> result;
    Node *current = search_leaf_node(left_key);
    if (current == nullptr)
    {
        return result;
    }
    int i = 0;
    while (current != nullptr)
    {
        for (i = 0; i < current->size && current->keys[i] <= right_key; i++)
        {
            result.push_back(current->record_values[i]);
        }
        if (current->keys[i] > right_key)
        {
            break;
        }
        current = current->next;
        i = 0;
    }
    return result;
};

std::pair<BPlusTree::Iterator, BPlusTree::Iterator> BPlusTree::search_range_iter(float left_key, float right_key)
{
    return {search_range_begin(left_key, right_key), search_range_end()};
};

void BPlusTree::insert(float key, Record *value)
{
    Node *root = this->root;
    if (root->size < this->degree)
    {
        root->insert(key, value);
    }
    else
    {
        // overflow
        std::cout << "Overflow occurred at root. Splitting root node." << std::endl;
        Node *new_root = new Node(degree, 0);
        new_root->node_values[0] = root;
        new_root->split_child(0);

        int i = (new_root->keys[0] < key) ? 1 : 0;
        new_root->node_values[i]->insert(key, value);
        this->root = new_root;
    }
};