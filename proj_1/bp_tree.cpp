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

void BPlusTree::insert(float key, Record *value)
{
    if (root-> is_leaf)
    {
        auto [new_child, new_key] = root->insert(key, value);
        if (new_child != nullptr)
        {
            Node *new_root = new Node(degree, 0);
            new_root->keys[0] = new_key;
            new_root->node_values[0] = root;
            new_root->node_values[1] = new_child;
            new_root->size = 2;
            root = new_root;
        }
    }
    else
    {
        auto [new_child, new_key] = root->insert(key, value);
        if (new_child == nullptr) {
            return;
        }

        Node *new_root = new Node(degree, 0);
        new_root->keys[0] = new_key;
        new_root->node_values[0] = root;
        new_root->node_values[1] = new_child;
        new_root->size = 2;
        root = new_root;

    }
};


Node::Node(int degree, bool is_leaf) : is_leaf(is_leaf), degree(degree)
{
    this->keys = new float[degree];
    if (this->is_leaf)
    {
        // this->record_values = new Record *[degree];
        // for (int i = 0; i < degree; i++)
        // {
        //     record_values[i] = nullptr;
        // }
        this->record_values.resize(degree);
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
    this->next = nullptr;
};

Node::~Node()
{
    delete[] keys;
    if (is_leaf)
    {
        // delete[] record_values;
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

std::pair<Node*, float> Node::insert(float key, Record *record)
{
    if (is_leaf)
    {
        // check if duplicate key
        for (int i = 0; i < size; i++)
        {
            if (keys[i] == key)
            {
                record_values[i].push_back(record);
                return {nullptr, 0};
            }
        }

        if (size < degree)
        {
            int i = size - 1;
            while (i >= 0 && keys[i] > key)
            {
                keys[i + 1] = keys[i];
                // record_values[i + 1] = record_values[i];
                i--;
            }
            keys[i + 1] = key;
            // record_values[i + 1] = record;
            record_values.at(i + 1).push_back(record);
            size++;
            return {nullptr, 0};
        }
        else
        {
            Node* sibling = split_leaf_child(key, record);
            return {sibling, sibling->keys[0]};
        }
    }
    else
    {
        int i = 0;
        while (i < size - 1 && keys[i] < key)
        {
            i++;
        }
        Node *child = node_values[i];
        auto [new_child, new_key] = child->insert(key, record);

        if (new_child == nullptr)
        {
            return {nullptr, 0};
        }

        // if current not full add new key
        if (size < degree + 1)
        {
            for (i = size - 2; i >= 0 && keys[i] > new_key; i--)
            {
                keys[i + 1] = keys[i];
                node_values[i + 2] = node_values[i + 1];
            }
            keys[i+1] = new_key;
            node_values[i + 2] = new_child;
            size++;
            return {nullptr, 0};
        }
        else
        {
            Node *sibling = split_internal_child(new_key, new_child);
            // shift all sibling keys by 1 to left and move left key up
            float left_key = sibling->keys[0];
            for (i = 0; i < sibling->size - 1; i++)
            {
                sibling->keys[i] = sibling->keys[i + 1];
            }
            return {sibling, left_key};
        }
    }
};

Node *Node::split_leaf_child(float key, Record *record)
{
    Node *sibling = new Node(degree, this->is_leaf);
    int split_index = ceil_div(degree+1, 2);

    if (key > keys[split_index - 1]) {
        for (int i = split_index; i < size; i++)
        {
            sibling->keys[i - split_index] = keys[i];
            sibling->record_values[i - split_index] = record_values[i];
            keys[i] = 0;
            record_values[i].clear();
        }
        int i = size - split_index - 1;
        while (i >= 0 && sibling->keys[i] > key)
        {
            sibling->keys[i + 1] = sibling->keys[i];
            sibling->record_values[i + 1] = sibling->record_values[i];
            i--;
        }
        sibling->keys[i + 1] = key;
        // sibling->record_values[i + 1] = record;
        sibling->record_values[i + 1].push_back(record);
        sibling->size = size - split_index + 1;
        size = split_index;
    }
    else
    {
        split_index--;
        for (int i = split_index; i < size; i++)
        {
            sibling->keys[i - split_index] = keys[i];
            sibling->record_values[i - split_index] = record_values[i];
            keys[i] = 0;
            record_values[i].clear();
        }
        // insert into current node
        int i = split_index - 1;
        while (i >= 0 && keys[i] > key)
        {
            keys[i + 1] = keys[i];
            record_values[i + 1] = record_values[i];
            i--;
        }
        keys[i + 1] = key;
        // record_values[i + 1] = record;
        record_values[i + 1].push_back(record);
        sibling->size = size - split_index;
        size = split_index + 1;
    }
    sibling->next = next;
    next = sibling;
    return sibling;
};

Node *Node::split_internal_child(float key, Node *record)
{
    Node *sibling = new Node(degree, this->is_leaf);
    int split_index = ceil_div(degree, 2);
    int i = 0;

    if (key > keys[split_index -1]) {
        // copy keys
        for (i = split_index; i < degree; i++)
        {
            sibling->keys[i - split_index] = keys[i];
            keys[i] = 0;
        }
        // copy node values
        for (i = split_index + 1; i < size; i++)
        {
            sibling->node_values[i - split_index - 1] = node_values[i];
            node_values[i] = nullptr;
        }
        // insert into sibling
        i = size - split_index - 2;
        while (i >= 0 && sibling->keys[i] > key)
        {
            sibling->keys[i + 1] = sibling->keys[i];
            sibling->node_values[i + 1] = sibling->node_values[i];
            i--;
        }
        sibling->keys[i + 1] = key;
        sibling->node_values[i + 1] = record;
        sibling->size = size - split_index;
        size = split_index + 1;
    }
    else
    {
        split_index--;
        // copy keys
        for (i = split_index; i < degree; i++)
        {
            sibling->keys[i - split_index] = keys[i];
            keys[i] = 0;
        }
        // copy node values
        for (i = split_index + 1; i < size; i++)
        {
            sibling->node_values[i - split_index - 1] = node_values[i];
            node_values[i] = nullptr;
        }
        // insert into current node
        i = split_index - 1;
        while (i >= 0 && keys[i] > key)
        {
            keys[i + 1] = keys[i];
            node_values[i + 2] = node_values[i + 1];
            i--;
        }
        keys[i + 1] = key;
        node_values[i + 2] = record;
        sibling->size = size - split_index - 1;
        size = split_index + 2;
    }
    return sibling;
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
    : current(node), index(index), right_key(right_key) {};

std::vector<Record *> BPlusTree::Iterator::operator*() const
{
    // return current->record_values[index].front();
    std::vector<Record *> result;
    for (Record *record : current->record_values[index])
    {
        result.push_back(record);
    }
    return result;
};

BPlusTree::Iterator &BPlusTree::Iterator::operator++()
{
    if (current == nullptr)
    {
        return *this;
    }
    index++;
    if (index >= current->size)
    {
        current = current->next;
        index = 0;
    }
    return *this;
};

bool BPlusTree::Iterator::operator!=(const Iterator &other) const
{
    return current != other.current && current->keys[index] <= right_key;
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
        while (i < current->size - 1 && current->keys[i] <= key)
        {
            i++;
        }
        current = current->node_values[i];
    }
    return current;
};

std::vector<Record *> BPlusTree::search(float key)
{
    Node *current = search_leaf_node(key);
    if (current == nullptr)
    {
        return std::vector<Record *>();
    }
    int index = get_index(key, current);
    if (index == -1)
    {
        return std::vector<Record *>();
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
            for (Record *record : current->record_values[i])
            {
                result.push_back(record);
            }
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
};

// void BPlusTree::print()
// {
//     if (root)
//     {
//         print_node(root, 0);
//     }
// };

void BPlusTree::print()
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

void BPlusTree::print_node(Node *node, int level)
{
    if (!node)
    {
        return;
    }

    std::cout << "Level " << level << ": ";

    if (node->is_leaf)
    {
        for (int i = 0; i < node->size; ++i)
        {
            std::cout << "(" << node->keys[i] << ") ";
        }
    }
    else
    {
        for (int i = 0; i < node->size - 1; ++i)
        {
            std::cout << "(" << node->keys[i] << ") ";
        }
    }

    std::cout << "| Size: " << node->size << std::endl;

    if (!node->is_leaf)
    {
         for (int i = 0; i < node->size; ++i)
         {
            print_node(node->node_values[i], level + 1);
         }
    }
}