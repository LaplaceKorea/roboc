#ifndef IDOCP_DYNAMIC_SCHUR_COMPLEMENT_HPP_
#define IDOCP_DYNAMIC_SCHUR_COMPLEMENT_HPP_

#include "Eigen/Dense"


namespace idocp {
///
/// @class DynamicSchurComplement
/// @brief The KKT matrix of a time stage.
///
class DynamicSchurComplement {
public:
  ///
  /// @brief Construct a KKT matrix.
  ///
  DynamicSchurComplement(const int max_dimA, const int max_dimD);

  ///
  /// @brief Default constructor. 
  ///
  DynamicSchurComplement();

  ///
  /// @brief Destructor. 
  ///
  ~DynamicSchurComplement();

  ///
  /// @brief Default copy constructor. 
  ///
  DynamicSchurComplement(const DynamicSchurComplement&) = default;

  ///
  /// @brief Default copy operator. 
  ///
  DynamicSchurComplement& operator=(const DynamicSchurComplement&) = default;
 
  ///
  /// @brief Default move constructor. 
  ///
  DynamicSchurComplement(DynamicSchurComplement&&) noexcept = default;

  ///
  /// @brief Default move assign operator. 
  ///
  DynamicSchurComplement& operator=(DynamicSchurComplement&&) noexcept = default;

  template <typename MatrixType1, typename MatrixType2, typename MatrixType3>
  void invertWithZeroBottomRightCorner(const Eigen::MatrixBase<MatrixType1>& A, 
                                       const Eigen::MatrixBase<MatrixType2>& C, 
                                       const Eigen::MatrixBase<MatrixType3>& Minv);

  template <typename MatrixType1, typename MatrixType2>
  void invertWithZeroBottomRightCorner(const Eigen::MatrixBase<MatrixType1>& C, 
                                       const Eigen::MatrixBase<MatrixType2>& Minv);

  template <typename MatrixType1, typename MatrixType2, typename MatrixType3>
  void invertWithZeroTopLeftCorner(const Eigen::MatrixBase<MatrixType1>& B,
                                   const Eigen::MatrixBase<MatrixType2>& D,
                                   const Eigen::MatrixBase<MatrixType3>& Minv);

  template <typename MatrixType1, typename MatrixType2>
  void invertWithZeroTopLeftCorner(const Eigen::MatrixBase<MatrixType1>& B,
                                   const Eigen::MatrixBase<MatrixType2>& Minv);

  EIGEN_MAKE_ALIGNED_OPERATOR_NEW

private:
  int max_dimA_, max_dimD_;
  Eigen::MatrixXd SA_, SD_, CAinv_, BDinv_;

};

} // namespace idocp 

#include "idocp/impulse/dynamic_schur_complement.hxx"

#endif // IDOCP_DYNAMIC_SCHUR_COMPLEMENT_HPP_ 