#include "tachyon/c/math/polynomials/univariate/bn254_univariate_rational_evaluations.h"

#include <vector>

#include "gtest/gtest.h"

#include "tachyon/base/containers/container_util.h"
#include "tachyon/c/math/elliptic_curves/bn/bn254/fr.h"
#include "tachyon/c/math/elliptic_curves/bn/bn254/fr_prime_field_traits.h"
#include "tachyon/cc/math/finite_fields/prime_field_conversions.h"
#include "tachyon/math/base/rational_field.h"
#include "tachyon/math/elliptic_curves/bn/bn254/fr.h"
#include "tachyon/math/polynomials/univariate/univariate_evaluations.h"

namespace tachyon::math {

namespace {

constexpr size_t kDegree = 5;

class UnivariateRationalEvaluationsTest : public testing::Test {
 public:
  constexpr static size_t kMaxDegree = SIZE_MAX;

  using Evals = UnivariateEvaluations<bn254::Fr, kMaxDegree>;
  using RationalEvals =
      UnivariateEvaluations<RationalField<bn254::Fr>, kMaxDegree>;

  static void SetUpTestSuite() { bn254::Fr::Init(); }

  void SetUp() override {
    RationalEvals* cpp_evals =
        new RationalEvals(RationalEvals::Random(kDegree));
    evals_ = reinterpret_cast<tachyon_bn254_univariate_rational_evaluations*>(
        cpp_evals);
  }

  void TearDown() override {
    tachyon_bn254_univariate_rational_evaluations_destroy(evals_);
  }

 protected:
  tachyon_bn254_univariate_rational_evaluations* evals_;
};

}  // namespace

TEST_F(UnivariateRationalEvaluationsTest, Clone) {
  tachyon_bn254_univariate_rational_evaluations* evals_clone =
      tachyon_bn254_univariate_rational_evaluations_clone(evals_);
  *reinterpret_cast<RationalEvals&>(*evals_)[0] +=
      RationalField<bn254::Fr>::One();
  EXPECT_NE((reinterpret_cast<RationalEvals&>(*evals_))[0],
            (reinterpret_cast<RationalEvals&>(*evals_clone))[0]);
  tachyon_bn254_univariate_rational_evaluations_destroy(evals_clone);
}

TEST_F(UnivariateRationalEvaluationsTest, Len) {
  EXPECT_EQ(tachyon_bn254_univariate_rational_evaluations_len(evals_),
            kDegree + 1);
}

TEST_F(UnivariateRationalEvaluationsTest, SetZero) {
  tachyon_bn254_univariate_rational_evaluations_set_zero(evals_, 0);
  EXPECT_TRUE(reinterpret_cast<RationalEvals&>(*evals_)[0]->IsZero());
}

TEST_F(UnivariateRationalEvaluationsTest, SetTrivial) {
  RationalField<bn254::Fr> expected =
      RationalField<bn254::Fr>(bn254::Fr::Random());
  tachyon_bn254_fr numerator = cc::math::ToCPrimeField(expected.numerator());
  tachyon_bn254_univariate_rational_evaluations_set_trivial(evals_, 0,
                                                            &numerator);
  EXPECT_EQ(*reinterpret_cast<RationalEvals&>(*evals_)[0], expected);
}

TEST_F(UnivariateRationalEvaluationsTest, SetRational) {
  RationalField<bn254::Fr> expected = RationalField<bn254::Fr>::Random();
  tachyon_bn254_fr numerator = cc::math::ToCPrimeField(expected.numerator());
  tachyon_bn254_fr denominator =
      cc::math::ToCPrimeField(expected.denominator());
  tachyon_bn254_univariate_rational_evaluations_set_rational(
      evals_, 0, &numerator, &denominator);
  EXPECT_EQ(*reinterpret_cast<RationalEvals&>(*evals_)[0], expected);
}

TEST_F(UnivariateRationalEvaluationsTest, BatchEvaluate) {
  std::vector<RationalField<bn254::Fr>> rational_values = base::CreateVector(
      kDegree + 1, []() { return RationalField<bn254::Fr>::Random(); });
  for (size_t i = 0; i < rational_values.size(); ++i) {
    tachyon_bn254_fr numerator =
        cc::math::ToCPrimeField(rational_values[i].numerator());
    tachyon_bn254_fr denominator =
        cc::math::ToCPrimeField(rational_values[i].denominator());
    tachyon_bn254_univariate_rational_evaluations_set_rational(
        evals_, i, &numerator, &denominator);
  }
  tachyon_bn254_univariate_evaluations* evaluated =
      tachyon_bn254_univariate_rational_evaluations_batch_evaluate(evals_);
  std::vector<bn254::Fr> values;
  values.resize(rational_values.size());
  CHECK(RationalField<bn254::Fr>::BatchEvaluate(rational_values, &values));
  EXPECT_EQ(reinterpret_cast<Evals&>(*evaluated).evaluations(), values);
}

}  // namespace tachyon::math
