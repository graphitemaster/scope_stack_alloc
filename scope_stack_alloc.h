#ifndef SCOPE_STACK_ALLOC_H
#define SCOPE_STACK_ALLOC_H

#include <functional>
#include <vector>
#include <new>

template <std::size_t E>
struct stack;

namespace detail {

template <typename T>
struct destructor {
    static void dtor(void *self);
};

class object {
    friend struct frame;

    template <std::size_t E>
    friend struct ::stack;

    object(void *self, void (*dtor)(void*));
    void *self;
    void (*dtor)(void *);
    void destroy();
};

class frame {
    template <std::size_t E>
    friend struct ::stack;

    frame(size_t index, unsigned char *sp);
    unsigned char *sp;
    std::size_t index;
    std::vector<detail::object> objects;
    void cleanup();
};

template <typename T>
void destructor<T>::dtor(void *self) {
    reinterpret_cast<T*>(self)->~T();
}

///! object
inline object::object(void *self, void (*dtor)(void*))
    : self(self)
    , dtor(dtor)
{
}

inline void object::destroy() {
    dtor(self);
}

///! frame
inline frame::frame(std::size_t index, unsigned char *sp)
    : index(index)
    , sp(sp)
{
}

inline void frame::cleanup() {
    for (auto it = objects.rbegin(); it != objects.rend(); ++it)
        (*it).destroy();
}

}

template <std::size_t E>
struct stack {
    stack() : sp(data) { }

    std::size_t enter() {
        frames.push_back(new detail::frame(frames.size(), sp));
        return frames.size();
    }

    void leave(std::size_t f = 0) {
        if (f >= frames.size()) return;
        detail::frame *get = frames[f];
        get->cleanup();
        frames.erase(frames.begin() + f);
        delete get;
    }

    unsigned char *allocate(size_t size, size_t alignment) {
        unsigned char *where = sp;
        const size_t base = reinterpret_cast<size_t>(where);
        sp += size + alignment;
        return reinterpret_cast<unsigned char *>(base + (alignment - base % alignment));
    }

    template <typename T>
    T &acquire() {
        T *obj = new(allocate(sizeof(T), alignof(T))) T();
        frames.back()->objects.push_back({static_cast<void*>(obj), &detail::destructor<T>::dtor});
        return *obj;
    }

    template <typename T, typename... Ts>
    T &acquire(Ts... ts) {
        T *obj = new(allocate(sizeof(T), alignof(T))) T(std::forward(ts...));
        frames.back()->objects.push_back({static_cast<void*>(obj), &detail::destructor<T>::dtor});
        return *obj;
    }

    void cleanup(std::size_t f = 0) {
        if (f + 1 >= frames.size()) return;
        for (auto it = frames.rbegin(); it != frames.rend() + f - 1; ++it) {
            (*it)->cleanup();
            delete *it;
        }
        frames.erase(frames.begin() + f + 1, frames.end());
        sp = frames[f]->sp;
    }

    stack &operator || (std::function<void()> scope) {
        enter();
        scope();
        leave();
        return *this;
    }

private:
    std::vector<detail::frame*> frames;
    alignas(alignof(void*)) unsigned char data[E];
    unsigned char *sp;
};

// Utility
template <typename T, std::size_t E>
T &acquire(stack<E> &s) {
    return s.template acquire<T>();
}

#endif
