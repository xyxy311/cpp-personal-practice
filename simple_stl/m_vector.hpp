#ifndef MY_VECTOR_HPP
#define MY_VECTOR_HPP

#include <initializer_list>
#include <memory>

namespace my {

template<typename T, typename Allocator = std::allocator<T>>
class vector {
public:
    // 成员类型
    using size_type = std::size_t;
    using allocator_type = Allocator;

    // 各种构造函数
    vector() = default;

    vector(const vector& v) 
        : m_allocator(v.m_allocator)    
    {
        m_data = allocate_raw(v.m_size);  // 不用按容量分配
        size_type i = 0;
        try {
            for (; i < v.m_size; ++i) {
                construct_at(m_data + i, v[i]);
            }
        }
        catch (...) {
            while (i > 0) {
                --i;
                destroy_at(m_data + i);
            }
            deallocate_raw(m_data, v.m_size);
            m_data = nullptr;
            throw;
        }

        m_capacity = m_size = v.m_size;  // 不用复制容量
    }
    
    // 使用初始化列表构造
    vector(std::initializer_list<T> ilist) {
        m_data = allocate_raw(ilist.size());
        size_type i = 0;
        try {
            for (; i < ilist.size(); ++i) {
                construct_at(m_data + i, ilist.begin()[i]);
            }
        }
        catch (...) {
            while (i > 0) {
                --i;
                destroy_at(m_data + i);
            }
            deallocate_raw(m_data, ilist.size());
            m_data = nullptr;
            throw;
        }
        m_capacity = m_size = ilist.size();
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

    ~vector() {   // 析构函数默认是 noexcept 的
        destroy_all();
        deallocate_raw(m_data, m_capacity);
    }

    // 交换函数
    void swap(vector& v) noexcept {
        std::swap(m_data, v.m_data);
        std::swap(m_size, v.m_size);
        std::swap(m_capacity, v.m_capacity);
    }

    // 索引访问
    T& operator[] (size_type index) noexcept { return m_data[index]; }
    const T& operator[] (size_type index) const noexcept { return m_data[index]; }  // 注意noexcept的位置

    // 属性接口
    size_type size() const noexcept { return m_size; }
    size_type capacity() const noexcept { return m_capacity; }
    bool empty() const noexcept { return m_size == 0; }
    
    // 其它操作
    template<typename U>
    void push_back(U&& elem) {
        grow_if_needed();
        // m_data[m_size++] = elem;  不能直接这样写
        construct_at(m_data + m_size, std::forward<U>(elem));  // 调用拷贝构造或移动构造
        ++m_size;  // 先构造，再递增，这样如果构造时抛出异常，也不会改变大小，回滚到之前的状态
    }

    template<typename... Args>
    T& emplace_back(Args&&... args) {
        grow_if_needed();
        construct_at(m_data + m_size, std::forward<Args>(args)...);  // 直接使用参数构造
        ++m_size;
        return m_data[m_size - 1];
    }

    void pop_back() noexcept {
        if (m_size > 0) {
            --m_size;
            destroy_at(m_data + m_size);
        }
    }

    void resize(size_type n) {
        resize_impl(n, [&](T* pos) {  // 注意捕获this指针
            construct_at(pos);
        });
    }

    void resize(size_type n, const T& val) {
        resize_impl(n, [&](T* pos) {
            construct_at(pos, val);    
        });
    }

    void reserve(size_type new_cap) {
        if (new_cap > m_capacity) {
            reallocate(new_cap);
        }
    }

    void clear() noexcept {
        destroy_all();
        m_size = 0;
    }
    
private:
    // 分配/释放 内存，不 构造/析构
    T* allocate_raw(size_type n) {  // 需要static吗
        return m_allocator.allocate(n);  // 不需要使用::operator new[]，这样会额外簿记数组个数
    }

    void deallocate_raw(T* p, size_type n) noexcept {
        m_allocator.deallocate(p, n);
    }

    // 构造元素
    template<typename... Args>  // 使用完美转发
    void construct_at(T* p, Args&&... args) {
        m_allocator.construct(p, std::forward<Args>(args)...);  // 注意 C++20 移除了 construct 成员函数
    }

    // 析构元素
    void destroy_at(T* p) noexcept {
        m_allocator.destroy(p);   // 标准要求析构时不抛出异常
    }
    void destroy_all() noexcept {
        for (size_type i = 0; i < m_size; ++i)
            destroy_at(m_data + i);
    }

    // 重新分配内存
    void reallocate(size_type new_capacity) {
        T* p = allocate_raw(new_capacity);
        size_type i = 0;  // 记录已构造的个数
        try {
            for (; i < m_size; ++i) {
                construct_at(p + i, std::move_if_noexcept(m_data[i]));
            }
        }
        catch (...) {
            // 回滚，析构已构造的新元素
            while (i > 0) {
                --i;
                destroy_at(p + i);
            }
            deallocate_raw(p, new_capacity);
            throw;
        }

        destroy_all();
        deallocate_raw(m_data, m_capacity);
        m_data = p;
        m_capacity = new_capacity;
    }

    // 扩容（如果没有闲置空间）
    void grow_if_needed() {
        if (m_size == m_capacity) {
            size_type new_capa = (m_capacity == 0) ? 10 : m_capacity * 2;
            reallocate(new_capa);
        }
    }

    template<typename Func>  // 这个函数用来构造
    void resize_impl(size_type n, Func construct_func) {
        if (n > m_size) {
            if (n > m_capacity) {
                size_type new_cap = std::max(n, 2 * m_capacity);
                reallocate(new_cap);
            }
            size_type old_size = m_size;
            try {
                for (; m_size < n; ++m_size) {
                    construct_func(m_data + m_size);
                }
            }
            catch (...) {
                while (m_size > old_size) {
                    --m_size;
                    destroy_at(m_data + m_size);
                }
                throw;
            }
        }
        else if (n < m_size) {
            for (; m_size > n; --m_size) {
                destroy_at(m_data + m_size - 1);
            }
        }
    }

    T* m_data = nullptr;   // C++11：就地初始化
    size_type m_size = 0;
    size_type m_capacity = 0;
    std::allocator<T> m_allocator;
};

} // namespace my


#endif