#include <cstdio>
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <type_traits>
using namespace std;

class A {
public:
    A();
private:
    ~A() = default;
};

int main()
{
    // A p(2);
    A *p = new A();
    free(p);
    return 0;
}
