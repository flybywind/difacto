#include "./sgd.h"
namespace difacto {

KWArgs SGDModel::Init(const KWArgs& kwargs, feaid_t start_id, feaid_t end_id) {
  CHECK_GT(end_id, start_id);
  start_id_ = start_id;
  end_id_ = end_id;
  if (end_id_ - start_id_ < 1e8) {
    dense_ = true;
    model_vec_.resize(end_id - start_id_);
  } else {
    dense_ = false;
  }
  return param_.InitAllowUnknown(kwargs);
}


void SGDModel::Load(dmlc::Stream* fi, bool* has_aux) {
  CHECK_NOTNULL(has_aux);
  CHECK_NOTNULL(fi);
  feaid_t id;
  std::vector<char> tmp((param_.V_dim*2+10)*sizeof(real_t));
  bool has_aux_cur, first = true;
  while (fi->Read(&id, sizeof(id))) {
    int len; fi->Read(&len);
    if (id < start_id_ || id >= end_id_) {
      // skip
      len = len > 0 : len : -len;
      CHECK_LT(len, (int)tmp.size());
      fi->Read(tmp.data(), len);
      continue;
    }
    // load
    id -= start_id_;
    if (dense_) {
      Load(fi, len, &model_vec_[id]);
    } else {
      Load(fi, len, &model_map_[id]);
    }
    // update has_aux
    has_aux_cur = len > 0;
    if (!first) CHECK_EQ(has_aux_cur, *has_aux);
    first = false;
    *has_aux = has_aux_cur;
  }
}

void SGDModel::Save(bool save_aux, dmlc::Stream *fo) const {
  if (dense_) {
    for (feaid_t id = 0; id < (feaid_t)model_vec_.size(); ++i) {
      Save(save_aux, id + start_id_, model_vec_[id], fo);
    }
  } else {
    for (const it& : model_) {
      Save(save_aux, it.first + start_id_, it.second, fo);
    }
  }
}

void SGDModel::Load(dmlc::Stream* fi, int len, SGDEntry* entry) {
  bool has_aux = len > 0;
  len = (len > 0 ? len : - len) / sizeof(real_t);

  CHECK_GE(len, 2);
  fi->Read(&entry->fea_cnt);
  fi->Read(&entry->w);
  len -= 2;

  if (has_aux) {
    CHECK_GE(len, 2);
    fi->Read(&entry->sqrt_g);
    fi->Read(&entry->z);
    len -= 2;
  }

  if (len > 0) {
    CHECK_EQ(len, param_.V_dim * (1 + has_aux));
    entry->V = new real_t[len];
    fi->Read(entry->V, len);
  }
}

void SGDModel::Save(bool save_aux, feaid_t id,
                    const SGDEntry& entry, dmlc::Stream *fo) {
  if (!save_aux && entry.V == nullptr && entry.w == 0) {
    // skip empty entry
    return;
  }
  int V_dim = param_.V_dim;
  int V_len = (1 + save_aux) * (entry->V ? V_dim : 0) * sizeof(real_t);
  int len = (1 + save_aux) * 2 * sizeof(real_t) + V_len;
  fo->Write(id);
  fo->Write(len);
  fo->Write(entry.fea_cnt);
  fo->Write(entry.w);
  if (save_aux) {
    fo->Write(entry.sqrt_g);
    fo->Write(entry.z);
  }
  if (V_len) fo->Write(entry.V, V_len);
}


void SGDOptimizer::Get(const std::vector<feaid_t>& fea_ids,
                       std::vector<T>* weights,
                       std::vector<int>* weight_lens) {
  int V_dim = param_.V_dim;
  size_t size = fea_ids.size();
  weights->resize(size * (1 + V_dim));
  weight_lens->resize(V_dim == 0 ? 0 : size);
  int p = 0;
  for (size_t i = 0; i < size; ++i) {
    auto& e = model_[fea_ids[i]];
    weights->at(p++) = e.w;
    if (e.V) {
      memcpy(weights->data()+p, e.V, V_dim*sizeof(real_t));
      p += V_dim;
    }
    if (V_dim != 0) {
      weight_lens->at(i) = (e.V ? V_dim : 0) + 1;
    }
  }
}

void SGDOptimizer::AddCount(const std::vector<feaid_t>& fea_ids,
                            const std::vector<real_t>& fea_cnts) {
  CHECK_EQ(fea_ids.size(), fea_cnts.size());
  for (size_t i = 0; i < fea_ids.size(); ++i) {
    auto& e = model_[fea_ids[i]];
    e.fea_cnt += fea_cnts[i];
    if (e.V == nullptr && e.w[0] != 0 && e.fea_cnt > param_.V_threshold) {
      InitV(&e);
    }
  }
}


void SGDOptimizer::Update(const std::vector<feaid_t>& fea_ids,
                          const std::vector<real_t>& grads,
                          const std::vector<int>& grad_lens) {
  CHECK(has_aux_) << "no aux data";
  size_t size = fea_ids.size();
  bool w_only = grad_lens.empty();
  if (w_only) {
    CHECK_EQ(grads.size(), size);
  }
  int p = 0;
  for (size_t i = 0; i < size; ++i) {
    auto& e = model_[fea_ids[i]];
    UpdateW(grads[p++], &e);
    if (!w_only && grad_lens[i] > 1) {
      CHECK_EQ(grad_lens[i], param_.V_dim);
      UpdateV(grads.data() + p, &e);
      p += param_.V_dim;
    }
  }
  CHECK_EQ((size_t)p, grads.size());
}


void SGDOptimizer::UpdateW(real_t gw, SGDEntry* e) {
  real_t sg = e->sqrt_g;
  real_t w = e->w;
  // update sqrt_g
  gw += w * param_.l2;
  e->sqrt_g = sqrt(sg * gs + gw * gw);
  // update z
  e->z -= gw - (e->sqrt_g - sg) / param_.lr * w;
  // update w by soft shrinkage
  real_t z = e->z;
  real_t l1 = param_.l1;
  if (z <= l1 && z >= - l1) {
    e->w = 0;
  } else {
    real_t eta = (param_.lr_beta + e->sqrt_g) / param_.lr;
    e->w = (z > 0 ? z - l1 : z + l1) / eta;
  }
  // update statistics
  if (w == 0 && e->w != 0) {
    ++ new_w_;
    if (e->V == nullptr && e->fea_cnt > param_.V_threshold) {
      InitV(e);
    }
  } else if (w != 0 && e->w[0] == 0) {
    -- new_w_;
  }
}

void SGDOptimizer::UpdateV(real_t const* gV, SGDEntry* e) {
  int n = param_.V_dim;
  for (int i = 0; i < n; ++i) {
    real_t g = gV[i] + param_.V_l2 * e->V[i];
    real_t cg = e->V[i+n];
    e->V[i+n] = sqrt(cg * cg + g * g);
    float eta = param_.V_lr / ( e->V[i+n] + param_.V_lr_beta);
    e->V[i] -= eta * g;
  }
}

void SGDOptimizer::InitV(SGDEntry* e) {
  int n = param_.V_dim;
  e->V = new real_t[n*2];
  for (int i = 0; i < n; ++i) {
    e->V[i] = (rand() / (real_t)RAND_MAX - 0.5) * param_.init_scale;
  }
  memset(e->V+n, 0, n*sizeof(real_t));
}

}  // namespace difacto
