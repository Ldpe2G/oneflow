#include "oneflow/core/operator/operator.h"

namespace oneflow {

class StackOp final : public Operator {
 public:
  OF_DISALLOW_COPY_AND_MOVE(StackOp);
  StackOp() = default;
  ~StackOp() = default;

  void InitFromOpConf() override {
    CHECK(op_conf().has_stack_conf());
    EnrollRepeatedInputBn("in");
    EnrollOutputBn("out");
  }

  Maybe<void> InferBlobDescs(std::function<BlobDesc*(const std::string&)> GetBlobDesc4BnInOp,
                             const ParallelContext* parallel_ctx) const override {
    const BlobDesc* in_0 = GetBlobDesc4BnInOp(input_bns().Get(0));
    std::vector<int64_t> shape_vec = in_0->shape().dim_vec();
    int64_t stack_axis = op_conf().stack_conf().axis();
    // if (stack_axis < 0) { stack_axis += shape_vec.size(); }
    OF_CHECK_GE(stack_axis, 0);
    OF_CHECK_LT(stack_axis, shape_vec.size());
    OF_CHECK_EQ(in_0->num_of_lod_levels(), 0);

    FOR_RANGE(size_t, i, 1, input_bns().size()) {
      const BlobDesc* in_i = GetBlobDesc4BnInOp(input_bns().Get(i));
      OF_CHECK_EQ(in_i->shape().NumAxes(), shape_vec.size());
      FOR_RANGE(int64_t, j, 0, in_i->shape().NumAxes()) { 
        if (j == stack_axis) {
          shape_vec.at(j) = std::max(shape_vec.at(j), in_i->shape().At(j));
        } else {
          OF_CHECK_EQ(in_i->shape().At(j), shape_vec.at(j));
        }
      }
      OF_CHECK_EQ(in_0->data_type(), in_i->data_type());
    }

    BlobDesc* out = GetBlobDesc4BnInOp("out");
    *out = *in_0;
    out->mut_shape() = Shape(shape_vec);
    return Maybe<void>::Ok();
  }

 private:
  Maybe<void> InferBatchAxis(
      const std::function<const BlobDesc&(const std::string&)>& LogicalBlobDesc4Ibn,
      std::function<OptInt64*(const std::string&)> BatchAxis4BnInOp) const override {
    return NaiveInferBatchAxis(BatchAxis4BnInOp);
  }

  Maybe<void> GetSbpSignatures(
      const std::function<Maybe<const BlobDesc*>(const std::string&)>& LogicalBlobDesc4Ibn,
      SbpSignatureList* sbp_sig_list) const override {
    int64_t stack_axis = op_conf().stack_conf().axis();
    int64_t num_axes = JUST(LogicalBlobDesc4Ibn(input_bns().Get(0)))->shape().NumAxes();
    FOR_RANGE(int64_t, i, 0, num_axes) {
      if (i == stack_axis) { continue; }
      SbpSignatureBuilder()
          .Split(input_bns(), i)
          .Split(output_bns(), i)
          .Build(sbp_sig_list->mutable_sbp_signature()->Add());
    }
    return Maybe<void>::Ok();
  }
};

REGISTER_OP(OperatorConf::kStackConf, StackOp);

}  // namespace oneflow
