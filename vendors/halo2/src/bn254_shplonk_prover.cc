#include "vendors/halo2/include/bn254_shplonk_prover.h"

#include "tachyon/base/buffer/buffer.h"
#include "tachyon/c/math/polynomials/univariate/bn254_univariate_evaluation_domain.h"
#include "tachyon/rs/base/rust_vec.h"
#include "vendors/halo2/src/bn254.rs.h"

namespace tachyon::halo2_api::bn254 {

SHPlonkProver::SHPlonkProver(uint32_t k, const Fr& s)
    : prover_(tachyon_halo2_bn254_shplonk_prover_create_from_unsafe_setup(
          TACHYON_HALO2_BLAKE_TRANSCRIPT, k,
          reinterpret_cast<const tachyon_bn254_fr*>(&s))) {}

SHPlonkProver::~SHPlonkProver() {
  tachyon_halo2_bn254_shplonk_prover_destroy(prover_);
}

uint32_t SHPlonkProver::k() const {
  return tachyon_halo2_bn254_shplonk_prover_get_k(prover_);
}

uint64_t SHPlonkProver::n() const {
  return static_cast<uint64_t>(
      tachyon_halo2_bn254_shplonk_prover_get_n(prover_));
}

rust::Box<G1JacobianPoint> SHPlonkProver::commit(const Poly& poly) const {
  return rust::Box<G1JacobianPoint>::from_raw(
      reinterpret_cast<G1JacobianPoint*>(
          tachyon_halo2_bn254_shplonk_prover_commit(prover_, poly.poly())));
}

rust::Box<G1JacobianPoint> SHPlonkProver::commit_lagrange(
    const Evals& evals) const {
  return rust::Box<G1JacobianPoint>::from_raw(
      reinterpret_cast<G1JacobianPoint*>(
          tachyon_halo2_bn254_shplonk_prover_commit_lagrange(prover_,
                                                             evals.evals())));
}

std::unique_ptr<Evals> SHPlonkProver::empty_evals() const {
  return std::make_unique<Evals>(
      tachyon_bn254_univariate_evaluation_domain_empty_evals(
          tachyon_halo2_bn254_shplonk_prover_get_domain(prover_)));
}

std::unique_ptr<RationalEvals> SHPlonkProver::empty_rational_evals() const {
  return std::make_unique<RationalEvals>(
      tachyon_bn254_univariate_evaluation_domain_empty_rational_evals(
          tachyon_halo2_bn254_shplonk_prover_get_domain(prover_)));
}

std::unique_ptr<Poly> SHPlonkProver::ifft(const Evals& evals) const {
  // NOTE(chokobole): The zero degrees might be removed. This might cause an
  // unexpected error if you use this carelessly. Since this is only used to
  // compute instance polynomial and this is used only in Tachyon side, so it's
  // fine.
  return std::make_unique<Poly>(tachyon_bn254_univariate_evaluation_domain_ifft(
      tachyon_halo2_bn254_shplonk_prover_get_domain(prover_), evals.evals()));
}

void SHPlonkProver::batch_evaluate(
    rust::Slice<const std::unique_ptr<RationalEvals>> rational_evals,
    rust::Slice<std::unique_ptr<Evals>> evals) const {
  for (size_t i = 0; i < rational_evals.size(); ++i) {
    evals[i] = std::make_unique<Evals>(
        tachyon_bn254_univariate_rational_evaluations_batch_evaluate(
            rational_evals[i]->evals()));
  }
}

void SHPlonkProver::set_rng(rust::Slice<const uint8_t> state) {
  tachyon_halo2_bn254_shplonk_prover_set_rng_state(prover_, state.data(),
                                                   state.size());
}

void SHPlonkProver::set_transcript(rust::Slice<const uint8_t> state) {
  tachyon_halo2_bn254_shplonk_prover_set_transcript_state(prover_, state.data(),
                                                          state.size());
}

void SHPlonkProver::set_extended_domain(const ProvingKey& pk) {
  tachyon_halo2_bn254_shplonk_prover_set_extended_domain(prover_, pk.pk());
}

void SHPlonkProver::create_proof(const ProvingKey& key,
                                 rust::Slice<InstanceSingle> instance_singles,
                                 rust::Slice<AdviceSingle> advice_singles,
                                 rust::Slice<const Fr> challenges) {
  tachyon_bn254_blinder* blinder =
      tachyon_halo2_bn254_shplonk_prover_get_blinder(prover_);
  const tachyon_bn254_plonk_verifying_key* vk =
      tachyon_bn254_plonk_proving_key_get_verifying_key(key.pk());
  const tachyon_bn254_plonk_constraint_system* cs =
      tachyon_bn254_plonk_verifying_key_get_constraint_system(vk);
  uint32_t blinding_factors =
      tachyon_bn254_plonk_constraint_system_compute_blinding_factors(cs);
  tachyon_halo2_bn254_blinder_set_blinding_factors(blinder, blinding_factors);

  size_t num_circuits = instance_singles.size();
  CHECK_EQ(num_circuits, advice_singles.size())
      << "size of |instance_singles| and |advice_singles| don't match";

  tachyon_halo2_bn254_argument_data* data =
      tachyon_halo2_bn254_argument_data_create(num_circuits);
  tachyon_halo2_bn254_argument_data_reserve_challenges(data, challenges.size());

  size_t num_bytes = base::EstimateSize(rs::RustVec());
  for (size_t i = 0; i < num_circuits; ++i) {
    uint8_t* buffer_ptr = reinterpret_cast<uint8_t*>(advice_singles.data());
    base::Buffer buffer(&buffer_ptr[num_bytes * 2 * i], num_bytes * 2);
    rs::RustVec vec;

    CHECK(buffer.Read(&vec));
    size_t num_advice_columns = vec.length;
    uintptr_t* advice_columns_ptr = reinterpret_cast<uintptr_t*>(vec.ptr);
    tachyon_halo2_bn254_argument_data_reserve_advice_columns(
        data, i, num_advice_columns);
    for (size_t j = 0; j < num_advice_columns; ++j) {
      tachyon_halo2_bn254_argument_data_add_advice_column(
          data, i, reinterpret_cast<Evals*>(advice_columns_ptr[j])->release());
    }

    CHECK(buffer.Read(&vec));
    size_t num_blinds = vec.length;
    const tachyon_bn254_fr* blinds_ptr =
        reinterpret_cast<const tachyon_bn254_fr*>(vec.ptr);
    tachyon_halo2_bn254_argument_data_reserve_advice_blinds(data, i,
                                                            num_blinds);
    for (size_t j = 0; j < num_blinds; ++j) {
      tachyon_halo2_bn254_argument_data_add_advice_blind(data, i,
                                                         &blinds_ptr[j]);
    }

    buffer_ptr = reinterpret_cast<uint8_t*>(instance_singles.data());
    buffer = base::Buffer(&buffer_ptr[num_bytes * 2 * i], num_bytes * 2);

    CHECK(buffer.Read(&vec));
    size_t num_instance_columns = vec.length;
    uintptr_t* instance_columns_ptr = reinterpret_cast<uintptr_t*>(vec.ptr);
    tachyon_halo2_bn254_argument_data_reserve_instance_columns(
        data, i, num_instance_columns);
    for (size_t j = 0; j < num_instance_columns; ++j) {
      tachyon_halo2_bn254_argument_data_add_instance_column(
          data, i,
          reinterpret_cast<Evals*>(instance_columns_ptr[j])->release());
    }

    CHECK(buffer.Read(&vec));
    CHECK_EQ(num_instance_columns, vec.length)
        << "size of instance columns don't match";
    uintptr_t* instance_poly_ptr = reinterpret_cast<uintptr_t*>(vec.ptr);
    tachyon_halo2_bn254_argument_data_reserve_instance_polys(
        data, i, num_instance_columns);
    for (size_t j = 0; j < num_instance_columns; ++j) {
      tachyon_halo2_bn254_argument_data_add_instance_poly(
          data, i, reinterpret_cast<Poly*>(instance_poly_ptr[j])->release());
    }
  }

  tachyon_halo2_bn254_shplonk_prover_create_proof(prover_, key.pk(), data);
  tachyon_halo2_bn254_argument_data_destroy(data);
}

rust::Vec<uint8_t> SHPlonkProver::get_proof() const {
  size_t proof_len;
  tachyon_halo2_bn254_shplonk_prover_get_proof(prover_, nullptr, &proof_len);
  rust::Vec<uint8_t> proof;
  // NOTE(chokobole): |rust::Vec<uint8_t>| doesn't have |resize()|.
  proof.reserve(proof_len);
  for (size_t i = 0; i < proof_len; ++i) {
    proof.push_back(0);
  }
  tachyon_halo2_bn254_shplonk_prover_get_proof(prover_, proof.data(),
                                               &proof_len);
  return proof;
}

std::unique_ptr<SHPlonkProver> new_shplonk_prover(uint32_t k, const Fr& s) {
  return std::make_unique<SHPlonkProver>(k, s);
}

rust::Box<Fr> ProvingKey::transcript_repr(const SHPlonkProver& prover) {
  tachyon_halo2_bn254_shplonk_prover_set_transcript_repr(prover.prover(), pk_);
  tachyon_bn254_fr* ret = new tachyon_bn254_fr;
  tachyon_bn254_fr repr = tachyon_bn254_plonk_verifying_key_get_transcript_repr(
      tachyon_bn254_plonk_proving_key_get_verifying_key(pk_));
  memcpy(ret->limbs, repr.limbs, sizeof(uint64_t) * 4);
  return rust::Box<Fr>::from_raw(reinterpret_cast<Fr*>(ret));
}

}  // namespace tachyon::halo2_api::bn254
