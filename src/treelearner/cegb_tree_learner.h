#ifndef LIGHTGBM_TREELEARNER_CEGB_TREE_LEARNER_H_
#define LIGHTGBM_TREELEARNER_CEGB_TREE_LEARNER_H_

#include <LightGBM/utils/array_args.h>
#include <LightGBM/utils/random.h>

#include <LightGBM/dataset.h>
#include <LightGBM/tree.h>
#include <LightGBM/tree_learner.h>

#include "data_partition.hpp"
#include "feature_histogram.hpp"
#include "leaf_splits.hpp"
#include "serial_tree_learner.h"
#include "split_info.hpp"

#include <cmath>
#include <cstdio>
#include <memory>
#include <random>
#include <vector>

namespace LightGBM {

class CEGBTreeLearner : public SerialTreeLearner {
public:
  CEGBTreeLearner(const TreeConfig *tree_config, const CEGBConfig *cegb_config_, std::vector<bool> &lazy_features_used_,
		  std::vector<bool> &coupled_features_used_, std::vector<int> &new_features_used_,
                  std::vector<data_size_t> &bag_data_indices_

                  )
      : SerialTreeLearner(tree_config), cegb_config(cegb_config_), predecessor(nullptr), lazy_features_used(lazy_features_used_),
        coupled_features_used(coupled_features_used_), new_features_used(new_features_used_), bag_data_indices(bag_data_indices_)

  {
    independent_branches = false;

    // GreedyMiser mode -> treat branches as independent even when using coupled
    // feature penalties
    if (cegb_config->independent_branches == true)
      independent_branches = true;

    // no coupled feature penalties -> all branches are independent since they
    // share no training instances
    if (cegb_config->penalty_feature_coupled.size() == 0)
      independent_branches = true;

    // no prediction cost penalty -> all branches are independent since they
    // share no training instances
    if (cegb_config->tradeoff == 0)
      independent_branches = true;

    need_lazy_features = false;
    for (auto &kv : cegb_config->penalty_feature_lazy) {
      if (kv.second > 0.0f) {
        need_lazy_features = true;
        break;
      }
    }

    used_new_coupled_feature = true;
  }

  ~CEGBTreeLearner() {}

  void ConnectTo(const CEGBTreeLearner* predecessor_) {
    predecessor = predecessor_;
  }

protected:
  void FindBestSplitsForLeaves() override;
  void FindBestThresholds() override;
  void Split(Tree *, int, int *, int *) override;
  Tree* Train(const score_t* , const score_t *, bool ) override;

private:
  const CEGBConfig *cegb_config;
  const CEGBTreeLearner *predecessor;
  std::vector<bool> &lazy_features_used;
  std::vector<bool> &coupled_features_used;
  std::vector<int> &new_features_used;

  bool independent_branches;
  bool used_new_coupled_feature;
  bool need_lazy_features;

  // bagging
  std::vector<data_size_t> &bag_data_indices;

  /*! \brief stores best thresholds for all feature for all leaves */
  std::map<int, std::vector<SplitInfo>> leaf_feature_splits;
  std::map<int, std::vector<double>> leaf_feature_penalty;

  double CalculateOndemandCosts(int, int);
  void FindBestSplitForLeaf(int);
  int GetRealDataIndex(int);
};

} // namespace LightGBM

#endif // LIGHTGBM_TREELEARNER_CEGB_TREE_LEARNER_H_
