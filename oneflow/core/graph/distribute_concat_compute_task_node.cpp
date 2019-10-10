#include "oneflow/core/graph/distribute_concat_compute_task_node.h"
#include "oneflow/core/graph/task_graph.h"
#include "oneflow/core/graph/logical_node.h"
#include "oneflow/core/operator/variable_op.h"

namespace oneflow {

bool DistributeConcatCompTaskNode::HasBackwardCompTaskNode() { return false; }

void DistributeConcatCompTaskNode::ProduceAllRegstsAndBindEdges() {
  ProduceRegst("out", true);
  ForEachOutDataEdge([&](TaskEdge* edge) { BindEdgeWithProducedRegst(edge, "out"); });
}

void DistributeConcatCompTaskNode::ConsumeAllRegsts() {
  size_t cnt = 0;
  ForEachInDataEdge([&](TaskEdge* edge) {
    cnt += 1;
    ConsumeRegst("in", edge->GetSoleRegst());
  });
  CHECK_EQ(cnt, 1);
}

bool DistributeConcatCompTaskNode::IsReadyForBuild() {
  return GetSoleConsumedRegst("in")->IsLocked();
}

void DistributeConcatCompTaskNode::BuildExecGphAndRegst() {
  BuildExecGphStructAndBindInRegst();
  BuildOutRegst();
  mut_exec_gph().TopoForEachNode([this](ExecNode* node) { node->InferBlobDescs(parallel_ctx()); });
}

void DistributeConcatCompTaskNode::BuildExecGphStructAndBindInRegst() {
  for (std::shared_ptr<const Operator> op : logical_node()->op_vec()) {
    ExecNode* cur_node = mut_exec_gph().NewNode();
    cur_node->mut_op() = op;
  }
  auto in_regst = GetSoleConsumedRegst("in");
  mut_exec_gph().ForEachNode([&](ExecNode* cur_node) {
    for (const std::string& ibn : cur_node->op()->input_bns()) {
      if (in_regst->HasLbi(cur_node->op()->BnInOp2Lbi(ibn))) {
        cur_node->BindBnWithRegst(ibn, in_regst);
      }
    }
  });
}

void DistributeConcatCompTaskNode::BuildOutRegst() {
  std::shared_ptr<RegstDesc> out_regst = GetProducedRegst("out");
  mut_exec_gph().ForEachNode([&](ExecNode* cur_node) {
    HashSet<LogicalBlobId> found_lbis;
    for (ExecEdge* out_edge : cur_node->out_edges()) { found_lbis.insert(out_edge->lbi()); }
    for (const std::string& obn : cur_node->op()->output_bns()) {
      out_regst->AddLbi(cur_node->op()->BnInOp2Lbi(obn));
      cur_node->BindBnWithRegst(obn, out_regst);
    }
  });
}

void DistributeConcatCompTaskNode::InferProducedDataRegstTimeShape() {
  *GetProducedRegst("out")->mut_data_regst_time_shape() =
      GetSoleConsumedRegst("in")->data_regst_time_shape();
}

}  // namespace oneflow
