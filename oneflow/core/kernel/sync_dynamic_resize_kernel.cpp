#include "oneflow/core/kernel/kernel.h"
#include "oneflow/core/register/register_desc.h"

namespace oneflow {

template<DeviceType device_type, typename T>
class SyncDynamicResizeKernel final : public KernelIf<device_type> {
 public:
  OF_DISALLOW_COPY_AND_MOVE(SyncDynamicResizeKernel);
  SyncDynamicResizeKernel() = default;
  ~SyncDynamicResizeKernel() override = default;

 private:
  bool IsKernelLaunchSynchronized() const override { return false; }
  void ForwardDataContent(const KernelCtx& ctx,
                          std::function<Blob*(const std::string&)> BnInOp2Blob) const override {
    const SyncDynamicResizeOpConf& conf = this->op_conf().sync_dynamic_resize_conf();
    CHECK_EQ(conf.axis(), 0);
    std::shared_ptr<int64_t> size_on_cpu(new int64_t);
    const Blob* in = BnInOp2Blob("in");
    const Blob* size = BnInOp2Blob("size");
    Blob* out = BnInOp2Blob("out");
    AutoMemcpy(ctx.device_ctx, out->mut_dptr(), in->dptr(), in->ByteSizeOfDataContentField(),
               out->mem_case(), in->mem_case());
    AutoMemcpy(ctx.device_ctx, size_on_cpu.get(), size->dptr(), sizeof(int64_t), MakeHostMemCase(),
               size->mem_case());
    ctx.device_ctx->AddCallBack([out, size_on_cpu, conf]() {
      Shape shape = out->dense_shape_view();
      shape.Set(conf.axis(), *size_on_cpu);
      out->dense_shape_mut_view().set_shape(shape);
    });
  }
};

}  // namespace oneflow
