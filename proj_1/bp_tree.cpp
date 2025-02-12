#include <vector>

#include "record.h"

constexpr int KEY_SIZE = 8;

int ceil_div(int a, int b)
{
    return (a + b - 1) / b;
}

int floor_div(int a, int b)
{
    return a / b;
}

struct Node
{
    Node(int degree, bool is_leaf) : is_leaf(is_leaf)
    {
        keys = new float[degree];
        if (is_leaf)
        {
            this->record_values = new Record *[degree];
            this->next = nullptr;
        }
        else
        {
            this->node_values = new Node *[degree];
        }
    };
    ~Node() {};
    void insert(float key, Record *record)
    {
        int i = 0;
        while (i < KEY_SIZE && keys[i] < key)
        {
            i++;
        }

        // if there is enough space, insert into node
        if (size < KEY_SIZE)
        {
            size++;
            // shift keys and values to the right
            for (int j = size - 1; j > i; j++)
            {
                keys[j] = keys[j + 1];
                values[j] = values[j + 1];
            }
            // insert target
            keys[i] = key;
            values[i] = *record;
            return nullptr;
        }

        // node is full, split into 2
        Node *new_node = new Node();
        new_node->next = next;
        next = new_node;
        // move half of the keys and values to the new node
        // note this is floor division
        for (int j = size / 2; j < size; j++)
        {
            new_node->keys[j - size / 2] = keys[j];
            new_node->values[j - size / 2] = values[j];
            new_node->size++;
        }
        size /= 2;
        // insert into the new node
        if (i < size)
        {
            insert(key, record);
        }
        else
        {
            new_node->insert(key, record);
        }
    };

    // bool is_root = 0;
    bool is_leaf = 0;
    int degree = 0;
    int size = 0; // current number of keys
    float *keys;
    union
    {
        Node **node_values;
        Record **record_values;
    };
    Node *next = nullptr;
};

class BPlusTree
{
public:
    BPlusTree(int degree)
    {
        this->degree = degree;
        this->root = nullptr;
    };
    ~BPlusTree();
    int get_index(float key, Node *node)
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
    Node *search_leaf_node(float key)
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
    Record *search(float key)
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
    std::vector<Record*> search_range(float left_key, float right_key)
    {
        std::vector<Record*> result;
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
    void insert(float key, Record *value)
    {
        if (root == nullptr)
        {
            root = new Node(degree, true);
            root->insert(key, value);
            return;
        }
        Node *current = search_leaf_node(key);
        current->insert(key, value);
    };

private:
    int degree;
    int height;
    int size;
    Node *root;
};