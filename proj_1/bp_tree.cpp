#include "storage/storage.h"
#include "bp_tree.h"
#include <vector>
#include <iostream>
#include <utility>

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
        for (int i = 0; i < degree + 1; ++i)
        {
            delete node_values[i];
        }
        delete[] node_values;
    }
};

Node *Node::insert(float key, Record *record)
{
    int i = this->size - 1;
    std::cout << "size of current node: " << this->size << std::endl;
    if (this->is_leaf)
    {
        if (this->size == this->get_child_degree())
        { // overflow
            std::cout << "Overflow occurred at leaf node. Splitting leaf node." << std::endl;
            Node *new_child = split_leaf_child(key);
            if (this->keys[this->size - 1] < key)
            {
                i = new_child->size - 1;
                while (i >= 0 && new_child->keys[i] > key)
                {
                    new_child->keys[i + 1] = new_child->keys[i];
                    new_child->record_values[i + 1] = new_child->record_values[i];
                    i--;
                }
                new_child->keys[i + 1] = key;
                new_child->record_values[i + 1] = record;
                new_child->size++;
            }
            else
            {
                i = this->size - 1;
                while (i >= 0 && this->keys[i] > key)
                {
                    this->keys[i + 1] = this->keys[i];
                    this->record_values[i + 1] = this->record_values[i];
                    i--;
                }
                this->keys[i + 1] = key;
                this->record_values[i + 1] = record;
                this->size++;
            }
            return new_child;
        }
        else
        {
            i = this->size - 1;
            while (i >= 0 && this->keys[i] > key)
            {
                this->keys[i + 1] = this->keys[i];
                this->record_values[i + 1] = this->record_values[i];
                i--;
            }
            this->keys[i + 1] = key;
            this->record_values[i + 1] = record;
            this->size++;
        }
    }
    else
    {
        std::cout << "Inserting into internal node." << std::endl;
        // insert into child node
        i = this->size - 1;
        while (i > 0 && this->keys[i - 1] > key)
        {
            i--;
        }
        Node *new_child = node_values[i]->insert(key, record);
        // insert into current node
        if (new_child != nullptr)
        {
            if (this->size == this->get_child_degree())
            { // overflow
                std::cout << "Overflow occurred at internal node. Splitting internal node." << std::endl;
                Node *new_internal_child = split_internal_child(key);
                if (this->keys[this->size - 1] < key)
                {
                    i = new_internal_child->size - 1;
                    while (i >= 0 && new_internal_child->keys[i - 1] > key)
                    {
                        new_internal_child->keys[i] = new_internal_child->keys[i - 1];
                        new_internal_child->node_values[i + 1] = new_internal_child->node_values[i];
                        i--;
                    }
                    new_internal_child->keys[i] = new_child->keys[0];
                    new_internal_child->node_values[i + 1] = new_child;
                    new_internal_child->size++;
                }
                else
                {
                    i = this->size - 1;
                    while (i >= 0 && this->keys[i - 1] > key)
                    {
                        this->keys[i] = this->keys[i - 1];
                        this->node_values[i + 1] = this->node_values[i];
                        i--;
                    }
                    this->keys[i] = new_child->keys[0];
                    this->node_values[i + 1] = new_child;
                    this->size++;
                }
                return new_internal_child;
            }
            else
            { // no overflow
                i = this->size - 1;
                while (i >= 0 && this->keys[i - 1] > key)
                {
                    this->keys[i] = this->keys[i - 1];
                    this->node_values[i + 1] = this->node_values[i];
                    i--;
                }
                this->keys[i] = new_child->keys[0];
                this->node_values[i + 1] = new_child;
                this->size++;
            }
        }
    }
    return nullptr;
};

Node *Node::split_leaf_child(float key)
{
    Node *new_child = new Node(this->degree, 1);
    int t = floor_div(this->degree + 1, 2);
    this->size -= t;
    new_child->size = t;
    int i = 0;
    if (key > this->keys[this->size + i])
    {
        t--;
        new_child->size--;
        this->size++;
    }
    for (i = 0; i < t; i++)
    {
        new_child->keys[i] = this->keys[this->size + i];
        this->keys[this->size + i] = 0;
        new_child->record_values[i] = this->record_values[this->size + i];
        this->record_values[this->size + i] = nullptr;
    }
    new_child->next = this->next;
    this->next = new_child;
    return new_child;
};

Node *Node::split_internal_child(float key)
{
    Node *new_child = new Node(this->degree, this->is_leaf);
    int t = 0;
    std::cout << "Splitting internal node. Degree " << this->degree << std::endl;
    t = floor_div(this->degree + 2, 2);
    this->size -= t;
    new_child->size = t;

    std::cout << "Size of left and right is " << this->size << " " << new_child->size << std::endl;

    // new this holds the second half of the keys
    // always asssume that key is inserted on left side
    // but this might not be the case
    int i = 0;
    if (key < this->keys[this->size + i])
    {
        t--;
        new_child->size--;
        this->size++;
    }
    for (i = 0; i < t - 1; i++)
    {
        new_child->keys[i] = this->keys[this->size + i];
        this->keys[this->size + i] = 0;
    }
    for (int i = 0; i < t; i++)
    {
        new_child->node_values[i] = this->node_values[this->size + i];
        this->node_values[this->size + i] = nullptr;
    }
    new_child->keys[0] = new_child->node_values[0]->keys[0];
    return new_child;
};

Node *Node::split_child(int index, float key)
{
    Node *child = node_values[index];
    Node *new_child = new Node(child->degree, child->is_leaf);
    int t = 0;
    std::cout << "Splitting internal node. Degree " << child->degree << std::endl;
    t = floor_div(child->degree + 2, 2);
    child->size -= t;
    new_child->size = t;

    std::cout << "Size of left and right is " << child->size << " " << new_child->size << std::endl;

    // new child holds the second half of the keys
    // always asssume that key is inserted on left side
    // but this might not be the case
    int i = 0;
    if (key < child->keys[child->size + i])
    {
        t--;
        new_child->size--;
        child->size++;
    }
    for (i = 0; i < t - 1; i++)
    {
        new_child->keys[i] = child->keys[child->size + i];
    }
    for (int i = 0; i < t; i++)
    {
        new_child->node_values[i] = child->node_values[child->size + i];
        child->node_values[child->size + i] = nullptr;
    }
    return new_child;
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
    return std::make_pair(search_range_begin(left_key, right_key), search_range_end());
}

void BPlusTree::insert(float key, Record *value)
{
    Node *root = this->root;
    Node *new_child = root->insert(key, value);
    if (new_child != nullptr)
    {
        Node *new_root = new Node(degree, 0);
        new_root->node_values[0] = root;
        new_root->node_values[1] = new_child;
        new_root->keys[0] = new_child->keys[0];
        new_root->size = 2;
        this->root = new_root;
    }
};

void BPlusTree::print() const
{
    int count = 0;
    std::vector<Node *> current_level;
    current_level.push_back(root);
    while (!current_level.empty())
    {
        std::vector<Node *> next_level;
        for (Node *node : current_level)
        {
            if (node == nullptr)
            {
                continue;
            }
            if (node->is_leaf)
            {
                for (int i = 0; i < node->size; i++)
                {
                    std::cout << node->keys[i] << " ";
                }
                std::cout << "| ";
                if (!node->is_leaf)
                {
                    for (int i = 0; i < node->size; i++)
                    {
                        next_level.push_back(node->node_values[i]);
                    }
                }
            }
            else
            {
                for (int i = 0; i < node->size - 1; i++)
                {
                    std::cout << node->keys[i] << " ";
                }
                std::cout << "| ";
                if (!node->is_leaf)
                {
                    for (int i = 0; i < node->size; i++)
                    {
                        next_level.push_back(node->node_values[i]);
                    }
                }
            }
        }
        std::cout << std::endl;
        current_level = next_level;
        count += 1;
        if (count > 10)
        {
            break;
        };
    }
};