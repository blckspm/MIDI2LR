#pragma once
//http://stackoverflow.com/questions/13695640/how-to-make-a-c11-stdunordered-set-of-stdweak-ptr
// Caution: these functions do not work once the object has been deleted (weak_ptr then is "0")
// Would cause problems in many situations, but not in this program, as using an unsorted container
// and only needing these functions to be correct during insertion into the container.
// C++17 will fix this issue.
//
// Also, when C++17 is around, not only remove WeakHash.h as unnecessary, but also start using
// weak_from_this(), rather than as currently making a weak_ptr from shared_from_this()

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