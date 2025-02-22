#include "tachyon/crypto/commitments/kzg/shplonk.h"

#include <memory>
#include <string>

#include "gtest/gtest.h"

#include "tachyon/crypto/commitments/test/bn254_kzg_polynomial_openings.h"
#include "tachyon/crypto/transcripts/simple_transcript.h"
#include "tachyon/math/elliptic_curves/bn/bn254/bn254.h"
#include "tachyon/math/elliptic_curves/bn/bn254/g1.h"
#include "tachyon/math/polynomials/univariate/univariate_evaluation_domain_factory.h"

namespace tachyon::crypto {
namespace {

class SHPlonkTest : public testing::Test {
 public:
  constexpr static size_t K = 4;
  constexpr static size_t N = size_t{1} << K;
  constexpr static size_t kMaxDegree = N - 1;

  using PCS =
      SHPlonk<math::bn254::BN254Curve, kMaxDegree, math::bn254::G1AffinePoint>;
  using F = PCS::Field;
  using Poly = PCS::Poly;
  using Commitment = PCS::Commitment;
  using Point = Poly::Point;

  static void SetUpTestSuite() { math::bn254::BN254Curve::Init(); }

  void SetUp() override {
    KZG<math::bn254::G1AffinePoint, kMaxDegree, math::bn254::G1AffinePoint> kzg;
    pcs_ = PCS(std::move(kzg));
    ASSERT_TRUE(pcs_.UnsafeSetup(N, F(2)));
  }

 protected:
  PCS pcs_;
};

}  // namespace

TEST_F(SHPlonkTest, CreateAndVerifyProof) {
  OwnedPolynomialOpenings<Poly, Commitment> owned_openings;
  std::string error;
  ASSERT_TRUE(LoadAndParseJson(
      base::FilePath(
          "tachyon/crypto/commitments/test/bn254_kzg_polynomial_openings.json"),
      &owned_openings, &error));
  ASSERT_TRUE(error.empty());

  SimpleTranscriptWriter<Commitment> writer((base::Uint8VectorBuffer()));
  std::vector<PolynomialOpening<Poly>> prover_openings =
      owned_openings.CreateProverOpenings();
  ASSERT_TRUE(pcs_.CreateOpeningProof(prover_openings, &writer));

  base::Buffer read_buf(writer.buffer().buffer(), writer.buffer().buffer_len());
  SimpleTranscriptReader<Commitment> reader(std::move(read_buf));
  std::vector<PolynomialOpening<Poly, Commitment>> verifier_openings =
      owned_openings.CreateVerifierOpenings();
  EXPECT_TRUE((pcs_.VerifyOpeningProof(verifier_openings, &reader)));
}

}  // namespace tachyon::crypto
