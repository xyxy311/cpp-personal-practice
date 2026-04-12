#include "m_vector.hpp"
#include <iostream>

using my::vector;

template<typename T>
void print(const vector<T>& v, char* s = "") {
    std::cout << s;
    std::cout << "元素：";
    for (std::size_t i = 0; i < v.size(); ++i)
        std::cout << v[i] << ' ';
    std::cout << '\n' << "大小：" << v.size();
    std::cout << '\n' << "容量：" << v.capacity() << std::endl;
}

int main() {
    vector<int> v{1, 2, 3};
    print(v, "v");

    vector<int> empty;
    print(empty, "空的empty");
    empty.push_back(2);
    empty.push_back(4);
    print(empty, "push后的empty");
    empty.reserve(12);
    print(empty, "reserve 12后的empty");


    for (std::size_t i = 0; i < 10; ++i) {
        empty.push_back(25+i);
    }
    print(empty, "empty");

    empty.resize(20, 2);
    print(empty, "empty扩容为20后");
    empty.resize(3);
    print(empty, "empty缩小为3后");

    auto copy(v);
    print(copy, "copy of v");
    copy.clear();
    print(copy, "copy cleared");

    auto moved(std::move(empty));
    print(moved, "v move from empty");
    print(empty, "empty");
}