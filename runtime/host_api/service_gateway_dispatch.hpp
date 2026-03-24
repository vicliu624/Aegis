#pragma once

#include <cstddef>
#include <cstdint>

#include "device/common/binding/service_binding_registry.hpp"

namespace aegis::runtime {

class ServiceGatewayDispatch {
public:
    explicit ServiceGatewayDispatch(device::ServiceBindingRegistry& services);

    int call(uint32_t domain,
             uint32_t op,
             const void* input,
             std::size_t input_size,
             void* output,
             std::size_t* output_size) const;

private:
    device::ServiceBindingRegistry& services_;
};

}  // namespace aegis::runtime
