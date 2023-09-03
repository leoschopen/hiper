#include <algorithm>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <vector>


int main()
{
    std::vector<int> vec = {1, 3, 5, 7, 9};

    // lower_bound
    auto it1 = std::lower_bound(vec.begin(), vec.end(), 5);

    // upper_bound
    auto it2 = std::upper_bound(vec.begin(), vec.end(), 5);

    // equal_range
    auto pair = std::equal_range(vec.begin(), vec.end(), 5);

    // binary_search
    bool exist = std::binary_search(vec.begin(), vec.end(), 5);

    std::cout << "lower_bound: " << *it1 << std::endl;                                 // 5
    std::cout << "upper_bound: " << *it2 << std::endl;                                 // 7
    std::cout << "equal_range: " << *pair.first << " " << *pair.second << std::endl;   // 5,7
    std::cout << "binary_search: " << exist << std::endl;                              // 1

    return 0;
}
