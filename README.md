# scope_stack_alloc

Scope stacks are a memory management tool allowing for a linear-memory layout
of arbitrary object hierarchies.

### Why
Many reasons

## Heap allocation is expensive
  * Complicated to manage
  * Bad cache locality

## Fragmentation
Most of the time you allocate objects with mixed life-times next to each other,
this leads to fragmentation
  * Each allocation has to traverse a *free-list*
    * Cache miss for each probed location
  * Large blocks disappear quickly

## Alternatives don't work
Linear allocators can over come this but they have a variety of limitations
  * Can't deal with resource cleanup
   * Need manual cleanup functions
     * Manually remember what resources to free
     * Error prone
     * RAII not possible
  * Tedious low-level boilerplate

Scope stacks overcome all these limitations
  * Is a linear allocator (backed by stack space)
  * Rewinds when a scope is left (calling destructors.)
  * Only allocates from topmost scope
  * Can rewind scopes as desired
    * Fine-grain control over nested object lifetimes
    * Safe `setjmp` and `longjmp` (won't leak objects.)
  * Easy thread-local scratch space
  * PIMPL idiom becomes more attractive
    * Faster too

### Using
Just include *"scope_stack_alloc.h"* and make a stack object.

### Example

    #include "scope_stack_alloc.h"

    static stack<(1<<20)> gStack; // 1MB

    int main() {
        gStack || [&]() {
            auto &it = gStack.acquire<std::vector<int>>(); // Or acquire<std::vector<int>>(gStack);
            it.push_back(100);
            /// ...
        };
    }

### Limitations
There are some limitations
  * Must know all object lifetimes and ownership when acquiring
  * Cannot hold onto pointers to acquired memory
    * Think of it like local objects in a function
      * Memory is no longer valid when frame is left

### Frames
The manual scope guard function `enter` returns a *frame-index* which can
be passed to `leave` to leave that scope. Similarly; `cleanup` optionally takes
a *frame-index*, all active frames from **one-past** this index passed will be
purged (all active objects of those frames will be destroyed.) This is useful for
quick *tear-down* situations where the approprate calls to `leave` were not
made causing the frame to leak resources (`longjmp` for example.)

### Resources
http://dice.se/publications/scope-stack-allocation/
