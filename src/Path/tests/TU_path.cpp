#include "Path.hpp"
#include <iostream>

int main()
{
    Path p("chemin");

    Node& n = p.addNode(Vector3f(1.0f, 2.0f, 3.0f));
    Node& n1 = p.addNode(Vector3f(1.0f, 2.0f, 3.0f));

    std::cout << n.id() << " " << n.path().id() << std::endl;
    std::cout << n1.id() << " " << n1.path().id() << std::endl;

    return 0;
}
