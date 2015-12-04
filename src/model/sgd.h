/**
 * @file   sgd.h
 * @brief  the stochastic gradient descent solver
 */
#ifndef DIFACTO_MODEL_SGD_H_
#define DIFACTO_MODEL_SGD_H_
#include "difacto/model.h"
#include "dmlc/parameter.h"
namespace difacto {

struct SGDModelParam : public dmlc::Parameter<SGDModelParam> {
  /** \brief the l1 regularizer for :math:`w`: :math:`\lambda_1 |w|_1` */
  float l1 = 1;
  /** \brief the l2 regularizer for :math:`w`: :math:`\lambda_2 \|w\|_2^2` */
  float l2 = 0;
  /** \brief the l2 regularizer for :math:`V`: :math:`\lambda_2 \|V_i\|_2^2` */
  float V_l2 = .01;

  DMLC_DECLARE_PARAMETER(SGDModelParam) {
    DMLC_DECLARE_FIELD(l1).set_range(0, 1e10);
    DMLC_DECLARE_FIELD(l2).set_range(0, 1e10);
    DMLC_DECLARE_FIELD(V_l2).set_range(0, 1e10);
  }
};

// struct SGDUpdateParam : public dmlc::Parameter<SGDUpdateParam> {
//   /** \brief the learning rate :math:`\eta` (or :math:`\alpha`) for :math:`w` */
//   float lr;
//   /** \brief learning rate :math:`\beta` */
//   float lr_beta;
//   /** \brief learning rate :math:`\eta` for :math:`V`. */
//   float V_lr;
//   /** \brief leanring rate :math:`\beta` for :math:`V`. */
//   float V_lr_beta;
//   /**
//    * \brief the scale to initialize V.
//    * namely V is initialized by uniform random number in
//    *   [-V_init_scale, +V_init_scale]
//    */
//   float V_init_scale;

// };

// /**
//  * \brief the weight entry for one feature
//  */
// struct SGDEntry {
//  public:
//   SGDEntry() : fea_cnt(0), V_len(0), V(nullptr) {
//     memset(w, 0, sizeof(T)*3);
//   }
//   ~SGDEntry() { delete [] V; }

//   /** \brief load from disk */
//   void Load(Stream* fi) {
//   }

//   /** \brief save to disk */
//   void Save(Stream *fo) const {
//   }

//   /** \brief init V */
//   void InitV(int len, T init_scale) {
//     CHECK_EQ(V_len, 0) << "already inited";
//     V_len = len;
//     V = new T[len * 2];
//     for (int i = 0; i < len; ++i) {
//       V[i] = (rand() / static_cast<T>(RAND_MAX) - 0.5) * init_scale;
//     }
//     memset(V+len, 0, len*sizeof(T));
//   }

//   /** \brief the number of appearence of this feature in the data so far */
//   uint32_t fea_cnt;
//   /** \brief the lenght of V */
//   int V_len;
//   /** \brief w and its aux data */
//   T w[3];
//   /** \brief V and its aux data */
//   T *V;
// };

// /**
//  * \brief store all weights
//  */
// template <typename T>
// class SGDModel {
//  public:
//   /**
//    * \brief create model
//    *
//    * @param start_id the minimal feature id
//    * @param end_id the maximal feature id
//    */
//   SGDModel(feaid_t start_id, feaid_t end_id) {
//     CHECK_GT(end_id, start_id);
//     start_id_ = start_id;
//     end_id_ = end_id;
//     if (end_id_ - start_id_ < 1e8) {
//       dense_ = true;
//       model_vec_.resize(end_id - start_id_);
//     } else {
//       dense_ = false;
//     }
//   }

//   /**
//    * \brief get the weight entry for a feature id
//    * \param id the feature id
//    */
//   // inline SGDEntry& get(feaid_t id) {
//   inline SGDEntry& operator[] (feaid_t id) {
//     CHECK_GE(id, start_id_);
//     return dense_ ? model_vec_[id - start_id_] : model_map_[id - start_id_];
//   }

//   /**
//    * \brief load model
//    * \param fi input stream
//    */
//   void Load(dmlc::Stream* fi) override {
//     feaid_t id;
//     while (fi->Read(&id, sizeof(id))) {
//       CHECK_GE(id, start_id_);
//       if (dense_) {
//         model_vec_[id - start_id_].Load(fi);
//       } else {
//         model_map_[id - start_id_].Load(fi);
//       }
//     }
//   }

//   /**
//    * \brief save model
//    * \param fo output stream
//    */
//   void Save(dmlc::Stream *fo) const override {
//     if (dense_) {
//       for (feaid_t id = 0; id < (feaid_t)model_vec_.size(); ++i) {
//         const auto& e = model_vec_[id];
//         if (e.w[0] == 0 && e.V_len == 0) continue;
//         fo->Write(&id, sizeof(id));
//         e.Save(fo);
//       }
//     } else {
//       for (const it& : model_) {
//         const auto& e = it.second;
//         if (e.w[0] == 0 && e.V_len == 0) continue;
//         fo->Write(&it.first, sizeof(it.first));
//         e.Save(fo);
//       }
//     }
//   }

//  private:
//   bool dense_;
//   feaid_t start_id_;
//   std::vector<SGDEntry<T>> model_vec_;
//   std::unordered_map<SGDEntry<T>> model_map_;
// };

// /**
//  * \brief sgd optimizer
//  *
//  * - w is updated by FTRL, which is a smooth version of adagrad works well with
//  *   the l1 regularizer
//  * - V is updated by adagrad
//  */
// template <typename T>
// class SGDOptimizer : public Model<T> {
//  public:
//   SGD(const Config& conf, feaid_t start_id, feaid_t end_id)
//       : model_(start_id, end_id) { }
//   virtual ~SGD() { }

//   /**
//    * \brief load model
//    * \param fi input stream
//    */
//   void Load(dmlc::Stream* fi) override { model_.Load(fi); }

//   /**
//    * \brief save model
//    * \param fo output stream
//    */
//   void Save(dmlc::Stream *fo) const override { model_.Save(fo); }

//   /**
//    * \brief get the weights on the given features
//    *
//    * @param fea_ids the list of feature ids
//    * @param weights the according weight on this feature ids, in format [w_0, V_0,
//    * w_1, V_1, ...]
//    * @param weight_lens the i-th element stores len([w_i, V_i]), could be empty
//    * if there is only w
//    */
//   void Get(const std::vector<feaid_t>& fea_ids,
//            std::vector<T>* weights,
//            std::vector<int>* weight_lens) override {
//     size_t size = fea_ids.size();
//     weights->resize(size * (1 + V_dim));
//     weight_lens->resize(V_dim == 0 ? 0 : size);
//     int p = 0;
//     for (size_t i = 0; i < size; ++i) {
//       auto& e = model_[fea_ids[i]];
//       weights->at(p++) = e.w[0];
//       for (int j = 0; j < e.V_len; ++j) {
//         weights->at(p++) = e.V[j];
//       }
//       if (V_dim_ != 0) {
//         weight_lens->at(i) = e.V_len + 1;
//       }
//     }
//   }

//   /**
//    * \brief add feature count
//    *
//    * @param fea_ids the list of feature ids
//    * @param fea_cnts the according counts
//    */
//   void AddCount(const std::vector<feaid_t>& fea_ids,
//                 const std::vector<uint32_t>& fea_cnts) override {
//     CHECK_EQ(fea_ids.size(), fea_cnts.size());
//     for (size_t i = 0; i < fea_ids.size(); ++i) {
//       auto& e = model_[fea_ids[i]];
//       e.fea_cnt += fea_cnts[i];
//       if (e.V_len == 0 && e.w[0] != 0 && e.fea_cnt > V_threshold_) {
//         e.InitV(V_dim_, V_init_scale_);
//       }
//     }
//   }

//   /**
//    * \brief update the weights given the gradients
//    *
//    * @param fea_ids the list of feature ids
//    * @param gradients the according gradients on this feature ids, in format [gw_0, gV_0,
//    * gw_1, gV_1, ...]
//    * @param gradient_lens the i-th element stores len([gw_i, gV_i]), could be empty
//    * if there is only w
//    */
//   void Update(const std::vector<ps::Key>& fea_ids,
//               const std::vector<T>& gradients,
//               const std::vector<int>& gradient_lens) override {
//     size_t size = fea_ids.size();
//     bool no_len = gradient_lens.empty();
//     if (no_len) { CHECK_EQ(gradients.size(), size); }

//     int p = 0;

//     for (size_t i = 0; i < size; ++i) {
//       auto& e = model_[fea_ids[i]];
//       UpdateW(gradients[p], &e);
//       if (!no_len && gradient_lens[i] > 1) {
//         int n = gradient_lens[i] - 1;
//         UpdateV(gradients.data() + p, n, &e);
//         p += n;
//       }
//     }
//     CHECK_EQ((size_t)p, gradients.size());
//   }

//  private:
//   /** \brief update w by FTRL */
//   void UpdateW(T gw, SGDEntry* e) {
//     T w = e->w[0];
//     T cg = e->w[1];
//     // update w[1]
//     gw += w * w_l2_;
//     e->w[1] = sqrt(cg * cg + gw * gw);
//     // update w[2]
//     e->w[2] -= gw - (e->w[1] - cg) / w_alpha_ * w;
//     // update w[0] by soft shrinkage
//     T z = e->w[2];
//     if (z <= w_l1_ && z >= - w_l1_) {
//       e->w[0] = 0;
//     } else {
//       T eta = (w_beta + e->w[1]) / w_alpha;
//       e->w[0] = (z > 0 ? z - w_l1_ : z + w_l1_) / eta;
//     }
//     // update statistics
//     if (w == 0 && e->w[0] != 0) {
//       ++ new_w_;
//       if (e.V_len == 0 && e.fea_cnt > V_threshold_) {
//         e.InitV(V_dim_, V_init_scale_);
//       }
//     } else if (w != 0 && e->w[0] == 0) {
//       -- new_w_;
//     }
//   }

//   /** \brief update V by adagrad */
//   void UpdateV(T const* gV, int n, SGDEntry* e) {
//     for (int i = 0; i < n; ++i) {
//       T g = gV[i] + V_l2_ * e->V[i];
//       T cg = e->V[i+len];
//       e->V[i+len] = sqrt(cg * cg + g * g);
//       float eta = V_alpha_ / ( e->V[i+len] + V_beta_ );
//       e->V[i] -= eta * g;
//     }
//   }
//   SGDModel<T> model_;

//   T w_alpha_, w_beta_, V_alpha_, V_beta_;
//   int64_t new_w_;
//   T V_init_scale_;
//   int V_dim_, V_threshold_;
// };
//

}  // namespace difacto
#endif  // DIFACTO_MODEL_SGD_H_