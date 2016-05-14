#pragma once
//http://stackoverflow.com/questions/13695640/how-to-make-a-c11-stdunordered-set-of-stdweak-ptr

template<typename T>
struct MyWeakPtrHash: public std::unary_function<std::weak_ptr<T>, size_t>
{
    size_t operator()(const std::weak_ptr<T>& wp) const
    {
        auto sp = wp.lock();
        return std::hash<decltype(sp)>()(sp);
    }
};

template<typename T>
struct MyWeakPtrEqual: public std::unary_function<std::weak_ptr<T>, bool>
{

    bool operator()(const std::weak_ptr<T>& left, const std::weak_ptr<T>& right) const
    {
        return !left.owner_before(right) && !right.owner_before(left);
    }
};