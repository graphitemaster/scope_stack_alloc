# scope_stack_alloc

**scope_tack_alloc** is a scoped stack allocator used for creating stack objects
who's destructors will be called when the scope is left.

### Using
Just include *"scope_stack_alloc.h"* and make a stack object.

### Examples

    #include "scope_stack_alloc.h"

    static stack<(1<<20)> gStack; // 1MB

    int main() {
        gStack || [&]() {
            auto &it = gStack.acquire<std::vector<int>>();
            it.push_back(100);
            /// ...
        };
    }

It's a well known limitation that *setjmp* and *longjmp* will ignore calling
destructors of local stack allocated objects, scope_stack_alloc can deal with
this by taking advantage of manual scope guard functions and `cleanup`

    #include <setjmp.h>
    #include "scope_stack_alloc.h"

    static jump_buf gJump;
    static stack<(1<<20)> gStack; // 1MB

    void test(bool jumpOut) {
        gStack.enter(); // Manual scope guard
        auto &it = gStack.acquire<std::vector<int>());
        it.push_back(100);
        // ...
        if (jumpOut)
            longjmp(gJump, 1); // gStack.leave won't be called
        gStack.leave(); // Manual scope guard
    }

    int main() {
        if (setjmp(gJump)) {
            // Manually cleanup left over stack contents missed by longjmp
            gStack.cleanup();
        } else {
            test(false); // Will return
            test(true); // Won't return
            // Unreachable ...
        }
    }

### Frames
The manual scope guard function `enter` returns a *frame* index which can
be passed to `leave` to leave that scope. Similarly; `cleanup` optionally takes
a frame index which it uses as the offset into all currently active frames to
purge them.
