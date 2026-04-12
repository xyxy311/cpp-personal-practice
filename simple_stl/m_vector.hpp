#ifndef MY_VECTOR_HPP
#define MY_VECTOR_HPP

#include <iostream>

namespace my {

template<typename T>
class vector {
public:
    // 各种构造函数
    vector() = default;

    vector(const vector& v) : m_capacity(v.m_size), m_size(v.m_size) {  // 不用复制容量
        T* p = allocate_raw(m_capacity);
        m_data = p;
        for (std::size_t i = 0; i < m_size; ++i) {
            construct_at(p + i, v[i]);
        }
    }
    
    // 使用初始化列表构造
    vector(std::initializer_list<T> ilist) {
        m_data = allocate_raw(ilist.size());
        m_capacity = m_size = ilist.size();
        for (std::size_t i = 0; i < m_size; ++i) {
            construct_at(m_data + i, ilist.begin()[i]);  // 访问初始化列表的每个元素
        }
    }

    vector(vector&& v) noexcept : vector() {  // 注意noexcept的位置
        swap(v);   // 使用swap简化
    }
    
    vector& operator=(const vector& v) {
        if (this != &v) {
            vector temp(v);
            swap(temp);   // 利用swap简化，不用手动析构，也不用重复写构造
        }
        return *this;
    }

    vector& operator=(vector&& v) noexcept {
        swap(v);
        return *this;
    }

    ~vector() {   // 析构函数默认是noexcept的
        destroy_all();
        deallocate_raw(m_data);
        /*
            不需要判断 m_data 是否为空
            对空指针调用 ::operator delete 安全
        */
    }

    // 交换函数
    void swap(vector& v) noexcept {
        std::swap(m_data, v.m_data);
        std::swap(m_size, v.m_size);
        std::swap(m_capacity, v.m_capacity);
    }

    // 索引访问
    T& operator[] (std::size_t index) { return m_data[index]; }
    const T& operator[] (std::size_t index) const { return m_data[index]; }

    // 属性接口
    std::size_t size() const { return m_size; }
    std::size_t capacity() const { return m_capacity; }
    bool empty() const { return m_size == 0; }
    
    // 其它操作
    template<typename U>
    void push_back(U&& elem) {
        grow_if_needed();
        // m_data[m_size++] = elem;  不能直接这样写
        construct_at(m_data + m_size, std::forward<U>(elem));  // 调用拷贝构造或移动构造
        ++m_size;
    }

    template<typename... Args>
    T& emplace_back(Args&&... args) {
        grow_if_needed();
        construct_at(m_data + m_size, std::forward<Args>(args)...);
        ++m_size;
        return m_data[m_size - 1];
    }

    void pop_back() {
        if (m_size > 0) {
            --m_size;
            destroy_at(m_data + m_size);
        }
    }

    void resize(std::size_t n) {
        resize_impl(n, [&](T* pos) {  // 注意捕获this指针
            construct_at(pos);
        });
    }

    void resize(std::size_t n, const T& val) {
        resize_impl(n, [&](T* pos) {
            construct_at(pos, val);    
        });
    }

    void reserve(size_t new_cap) {
        if (new_cap > m_capacity) {
            reallocate(new_cap);
        }
    }

    void clear() {
        destroy_all();
        m_size = 0;
    }
    
private:
    // 分配/释放 内存，不 构造/析构
    static T* allocate_raw(std::size_t n) {  // 类级别的成员函数，不需要this指针，声明为static
        return static_cast<T*>(::operator new(n * sizeof(T)));  // 不需要使用::operator new[]，这样会额外簿记数组个数
    }

    static void deallocate_raw(T* p) {
        ::operator delete(p);
    }

    // 构造元素
    void construct_at(T* p) {
        new (p) T();  // placement new
    }

    template<typename... Args>  // 完美转发
    void construct_at(T* p, Args&&... args) {
        new (p) T(std::forward<Args>(args)...);  // placement new
    }

    // 析构元素
    void destroy_at(T* p) {
        p->~T();   // 显式调用析构函数
    }
    void destroy_all() {
        for (std::size_t i = 0; i < m_size; ++i)
            destroy_at(m_data + i);
    }

    // 重新分配内存
    void reallocate(std::size_t capacity) {
        T* p = allocate_raw(capacity);
        for (std::size_t i = 0; i < m_size; ++i) {
            construct_at(p + i, std::move_if_noexcept(m_data[i]));
        }
        destroy_all();
        deallocate_raw(m_data);
        m_data = p;
        m_capacity = capacity;
    }

    // 扩容（如果没有闲置空间）
    void grow_if_needed() {
        if (m_size == m_capacity) {
            std::size_t new_capa = (m_capacity == 0) ? 10 : m_capacity * 2;
            reallocate(new_capa);
        }
    }

    template<typename Func>  // 这个函数用来构造
    void resize_impl(std::size_t n, Func construct_func) {
        if (n > m_size) {
            if (n > m_capacity) {
                std::size_t new_cap = std::max(n, 2 * n);
                reallocate(new_cap);
            }
            for (; m_size < n; ++m_size) {
                construct_func(m_data + m_size);
            }
        }
        else if (n < m_size) {
            for (; m_size > n; --m_size) {
                destroy_at(m_data + m_size - 1);
            }
        }
    }

    T* m_data = nullptr;   // C++11：就地初始化
    std::size_t m_size = 0;
    std::size_t m_capacity = 0;
};

} // namespace my


#endif