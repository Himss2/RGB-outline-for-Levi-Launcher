#include "outline/runtime.hpp"

#include <atomic>

namespace outline::runtime {

namespace {

std::atomic<void*> gClientInstance{
    nullptr
};

}

void setClientInstance(void* value) {
    gClientInstance.store(
        value,
        std::memory_order_release
    );
}

void* clientInstance() {
    return gClientInstance.load(
        std::memory_order_acquire
    );
}

}
