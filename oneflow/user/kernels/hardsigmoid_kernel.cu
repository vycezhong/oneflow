/*
Copyright 2020 The OneFlow Authors. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#include "oneflow/core/framework/framework.h"
#include "oneflow/core/common/data_type.h"
#include "oneflow/core/kernel/util/cuda_half_util.h"
#include "oneflow/core/cuda/elementwise.cuh"
namespace oneflow {

namespace user_op {

template<typename T>
struct HardsigmoidFunctor {
  OF_DEVICE_FUNC T operator()(T x) const {
    if (x <= static_cast<T>(-3))
      return static_cast<T>(0);
    else if (x >= static_cast<T>(3))
      return static_cast<T>(1);
    else
      return (x / static_cast<T>(6)) + static_cast<T>(0.5);
  }
};

template<typename T>
struct HardsigmoidGradFunctor {
  OF_DEVICE_FUNC T operator()(T x, T dy) const {
    return (x > static_cast<T>(-3) && x < static_cast<T>(3)) ? dy / static_cast<T>(6)
                                                             : static_cast<T>(0);
  }
};

template<>
struct HardsigmoidFunctor<half> {
  HardsigmoidFunctor<float> float_functor;
  OF_DEVICE_FUNC half operator()(half x) const {
    return __float2half(float_functor(__half2float(x)));
  }
};

template<>
struct HardsigmoidGradFunctor<half> {
  HardsigmoidGradFunctor<float> float_functor;
  OF_DEVICE_FUNC half operator()(half x, half dy) const {
    return __float2half(float_functor(__half2float(x), __half2float(dy)));
  }
};

template<DeviceType device_type, typename T>
class GpuHardsigmoidKernel final : public OpKernel {
 public:
  GpuHardsigmoidKernel() = default;
  ~GpuHardsigmoidKernel() = default;

 private:
  void Compute(KernelComputeContext* ctx) const override {
    const Tensor* in_tensor = ctx->Tensor4ArgNameAndIndex("in", 0);
    Tensor* out_tensor = ctx->Tensor4ArgNameAndIndex("out", 0);
    const T* in_ptr = in_tensor->dptr<T>();
    T* out_ptr = out_tensor->mut_dptr<T>();
    const int32_t elem_cnt = in_tensor->shape().elem_cnt();
    OF_CUDA_CHECK((oneflow::cuda::elementwise::Unary(HardsigmoidFunctor<T>(), elem_cnt, out_ptr,
                                                     in_ptr, ctx->device_ctx()->cuda_stream())));
  }
  bool AlwaysComputeWhenAllOutputsEmpty() const override { return false; }
};

#define REGISTER_GPU_HARDSIGMOID_KERNEL(device, dtype)    \
  REGISTER_USER_KERNEL("hardsigmoid")                     \
      .SetCreateFn<GpuHardsigmoidKernel<device, dtype>>() \
      .SetIsMatchedHob((HobDeviceTag() == device)         \
                       & (HobDataType("out", 0) == GetDataType<dtype>::value));

REGISTER_GPU_HARDSIGMOID_KERNEL(DeviceType::kGPU, half);
REGISTER_GPU_HARDSIGMOID_KERNEL(DeviceType::kGPU, float);
REGISTER_GPU_HARDSIGMOID_KERNEL(DeviceType::kGPU, double);

template<DeviceType device_type, typename T>
class GpuHardsigmoidGradKernel final : public OpKernel {
 public:
  GpuHardsigmoidGradKernel() = default;
  ~GpuHardsigmoidGradKernel() = default;

 private:
  void Compute(KernelComputeContext* ctx) const override {
    const Tensor* x_tensor = ctx->Tensor4ArgNameAndIndex("x", 0);
    const Tensor* dy_tensor = ctx->Tensor4ArgNameAndIndex("dy", 0);
    Tensor* dx_tensor = ctx->Tensor4ArgNameAndIndex("dx", 0);
    const T* x_ptr = x_tensor->dptr<T>();
    const T* dy_ptr = dy_tensor->dptr<T>();
    T* dx_ptr = dx_tensor->mut_dptr<T>();

    const int32_t elem_cnt = x_tensor->shape().elem_cnt();
    OF_CUDA_CHECK(
        (oneflow::cuda::elementwise::Binary(HardsigmoidGradFunctor<T>(), elem_cnt, dx_ptr, x_ptr,
                                            dy_ptr, ctx->device_ctx()->cuda_stream())));
  }
  bool AlwaysComputeWhenAllOutputsEmpty() const override { return false; }
};

#define REGISTER_GPU_HARDSIGMOID_BACKWARD_KERNEL(device, dtype) \
  REGISTER_USER_KERNEL("hardsigmoid_grad")                      \
      .SetCreateFn<GpuHardsigmoidGradKernel<device, dtype>>()   \
      .SetIsMatchedHob((HobDeviceTag() == device)               \
                       & (HobDataType("dx", 0) == GetDataType<dtype>::value));

REGISTER_GPU_HARDSIGMOID_BACKWARD_KERNEL(DeviceType::kGPU, half);
REGISTER_GPU_HARDSIGMOID_BACKWARD_KERNEL(DeviceType::kGPU, float);
REGISTER_GPU_HARDSIGMOID_BACKWARD_KERNEL(DeviceType::kGPU, double);

}  // namespace user_op

}  // namespace oneflow
