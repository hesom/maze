#include <iostream>
#include <vector>
#include <algorithm>

enum class GridCell : int
{
    EMPTY,
    WALL,
    FINISH,
    SPAWN
};

struct Point
{
    float x;
    float y;

    Point(float x, float y)
        :x(x), y(y)
    {

    }
};

struct RenderObject
{
    Point position = Point(0.0, 0.0);
};

struct Node
{
    RenderObject data;
    bool isLeaf;
    int axis;
    float border;
    Node* left;
    Node* right;
};

Node* kdTree(std::vector<RenderObject>& objects, int depth = 0)
{
    if (objects.size() == 0) return nullptr;
    if(objects.size() == 1)
    {
        Node* root = new Node;
        root->data = objects.at(0);
        root->isLeaf = true;
        root->left = nullptr;
        root->right = nullptr;
        return root;
    }
    int axis = depth % 2;

    std::sort(objects.begin(), objects.end(), [&axis](RenderObject a, RenderObject b) {
        if (axis == 0) {
            return a.position.x < b.position.x;
        } else {
            return a.position.y < b.position.y;
        }
    });
    RenderObject median = objects.at(objects.size() / 2);
    Node* root = new Node;
    root->axis = axis;
    root->isLeaf = false;
    if (axis == 0) {
        root->border = median.position.x;
    } else {
        root->border = median.position.y;
    }
    std::vector<RenderObject> left(&objects[0], &objects[objects.size() / 2]);
    std::vector<RenderObject> right(&objects[objects.size() / 2], &objects.back() + 1);
    root->left = kdTree(left, depth + 1);
    root->right = kdTree(right, depth + 1);
    return root;
}

void freeTree(Node* root){
    if (root == nullptr) {
        return;
    }
    freeTree(root->left);
    freeTree(root->right);
    delete root;
    root = nullptr;
}

void printTree(Node* root)
{
    if (root == nullptr) {
        return;
    }

    printTree(root->left);

    if (root->isLeaf) {
        std::cout << "(" << root->data.position.x << "," << root->data.position.y << ")" << std::endl;
    }

    printTree(root->right);
}


int main(int argc, char** argv)
{

    RenderObject a;
    a.position = Point{ 2.0f, 3.0f };
    RenderObject b;
    b.position = Point{ 5.0f, 4.0f };
    RenderObject c;
    c.position = Point{ 9.0f, 6.0f };
    RenderObject d;
    d.position = Point{ 4.0f, 7.0f };
    RenderObject e;
    e.position = Point{ 8.0f, 1.0f };
    RenderObject f;
    f.position = Point{ 7.0f, 2.0f };

    std::vector<RenderObject> objects;
    objects.push_back(a);
    objects.push_back(b);
    objects.push_back(c);
    objects.push_back(d);
    objects.push_back(e);
    objects.push_back(f);

    Node* kdTreeRoot = kdTree(objects);
    printTree(kdTreeRoot);
    freeTree(kdTreeRoot);

    std::cin >> new char[0];
    return 0;
}