#include "gtest/gtest.h"

#include "tachyon/c/math/elliptic_curves/bn/bn254/fq_prime_field_traits.h"
#include "tachyon/c/math/elliptic_curves/bn/bn254/g1.h"
#include "tachyon/c/math/elliptic_curves/bn/bn254/g1_point_traits.h"
#include "tachyon/cc/math/elliptic_curves/point_conversions.h"
#include "tachyon/math/elliptic_curves/bn/bn254/g1.h"

namespace tachyon::c::math {

namespace {

class AffinePointTest : public testing::Test {
 public:
  static void SetUpTestSuite() {
    tachyon::math::bn254::G1AffinePoint::Curve::Init();
  }

  void SetUp() override {
    a_ = tachyon::math::bn254::G1AffinePoint::Random();
    b_ = tachyon::math::bn254::G1AffinePoint::Random();

    c_a_ = cc::math::ToCAffinePoint(a_);
    c_b_ = cc::math::ToCAffinePoint(b_);
  }

 protected:
  tachyon::math::bn254::G1AffinePoint a_;
  tachyon::math::bn254::G1AffinePoint b_;
  tachyon_bn254_g1_affine c_a_;
  tachyon_bn254_g1_affine c_b_;
};

}  // namespace

TEST_F(AffinePointTest, Zero) {
  tachyon_bn254_g1_affine c_ret = tachyon_bn254_g1_affine_zero();
  EXPECT_TRUE(cc::math::ToAffinePoint(c_ret).IsZero());
}

TEST_F(AffinePointTest, Generator) {
  tachyon_bn254_g1_affine c_ret = tachyon_bn254_g1_affine_generator();
  EXPECT_EQ(cc::math::ToAffinePoint(c_ret),
            tachyon::math::bn254::G1AffinePoint::Generator());
}

TEST_F(AffinePointTest, Random) {
  tachyon_bn254_g1_affine c_ret = tachyon_bn254_g1_affine_random();
  EXPECT_NE(cc::math::ToAffinePoint(c_ret), a_);
}

}  // namespace tachyon::c::math
