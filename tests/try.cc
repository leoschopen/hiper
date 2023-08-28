#include <cstdio>
#include <iostream>
#include <memory>
#include <string>

class Example {
public:
    Example(int value) : data(value) {}

    void printData() {
        std::cout << "Data: " << data << std::endl;
    }

private:
    int data;
};

std::unique_ptr<Example> createExample(int value) {
    return std::make_unique<Example>(value);
}


int main() {
    // std::unique_ptr<Example> ptr = createExample(42);
    // ptr->printData(); // 安全地访问对象

    // char * a = "123";
    std::string a = "123";
    auto p1 = std::make_shared<std::string>(a);
    printf("%s\n", p1->c_str());

    return 0;
}
