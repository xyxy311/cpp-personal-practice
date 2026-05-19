#include <cstddef>
#include <utility>

namespace my_simple_stl {

template<typename T>
class allocator {
public:
    using value_type = T;
    using size_type = std::size_t;

    [[nodiscard]] static value_type* allocate(size_type n) {   // nodicard 用来警告没有接受返回值的调用行为
        if (n == 0) {
            return nullptr;
        }
        return static_cast<value_type*> (::operator new(n * sizeof(value_type)));
    }

    static void deallocate(value_type* pos, size_type n) noexcept {  // 忽略 n
        ::operator delete(static_cast<void*>(pos));
    }

    template<typename... Args>
    static void construct(value_type* pos, Args&&... args) {
        new (pos) value_type(std::forward<Args>(args)...);
    }

    static void destroy(value_type* pos) noexcept {
        pos->~value_type();
    }
};

}  // namespace my_simple_stl

