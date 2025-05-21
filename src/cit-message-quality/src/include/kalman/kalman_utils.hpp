#ifndef KALMAN_UTILS_HPP_
#define KALMAN_UTILS_HPP_

#include "kalman/include/kalman/LinearizedMeasurementModel.hpp"
#include "kalman/include/kalman/LinearizedSystemModel.hpp"
#include "kalman/include/kalman/UnscentedKalmanFilter.hpp"
#include <Eigen/Eigen>

/**
 * @brief State vector for UKF predictor implementation.
 */
template <typename T> class State : public Kalman::Vector<T, 6> {
  public:
    KALMAN_VECTOR(State, T, 6)

    // X-position
    static constexpr size_t X = 0;
    // Y-Position
    static constexpr size_t Y = 1;
    // Orientation
    static constexpr size_t THETA = 2;
    // X velocity
    static constexpr size_t V = 3;
    // Y velocity
    static constexpr size_t A = 4;
    // Angular velocity
    static constexpr size_t OMEGA = 5;

    T x() const { return (*this)[X]; }
    T y() const { return (*this)[Y]; }
    T theta() const { return (*this)[THETA]; }
    T v() const { return (*this)[V]; }
    T a() const { return (*this)[A]; }
    T omega() const { return (*this)[OMEGA]; }

    T &x() { return (*this)[X]; }
    T &y() { return (*this)[Y]; }
    T &theta() { return (*this)[THETA]; }
    T &v() { return (*this)[V]; }
    T &a() { return (*this)[A]; }
    T &omega() { return (*this)[OMEGA]; }
};

/**
 * @brief System control-input vector-type for CTRA motion model
 *
 *
 * @param T Numeric scalar type
 */
template <typename T> class Control : public Kalman::Vector<T, 1> {
  public:
    KALMAN_VECTOR(Control, T, 1)

    // time since filter was last called
    static constexpr size_t DT = 0;

    T dt() const { return (*this)[DT]; }
    T &dt() { return (*this)[DT]; }
};

template <typename T,
          template <class> class CovarianceBase = Kalman::StandardBase>
class SystemModel : public Kalman::LinearizedSystemModel<State<T>, Control<T>,
                                                         CovarianceBase> {
  public:
    // State type shortcut definition
    typedef State<T> S;

    // Control type shortcut definition
    typedef Control<T> C;

    /**
     * @brief Definition of (non-linear) state transition function
     *
     * This function defines how the system state is propagated through time,
     * i.e. it defines in which state \f$\hat{x}_{k+1}\f$ is system is expected
     * to be in time-step \f$k+1\f$ given the current state \f$x_k\f$ in step
     * \f$k\f$ and the system control input \f$u\f$.
     *
     * @param x The system state in current time-step
     * @param u The control vector input
     * @returns The predicted system state in the next time-step
     */
    S f(const S &x, const C &u) const {
        //! Predicted state vector after transition
        S x_;

        auto th = x.theta();
        auto v = x.v();
        auto a = x.a();
        auto om = x.omega();
        auto dT = u.dt();

        auto cosTh = std::cos(th);
        auto sinTh = std::sin(th);

        if (std::abs(om) < T(2.0)) {
            if (om < 0) 
              om = -2.0;
            else
              om = 2.0;            
        } else {
            auto cosThOmT = std::cos(th + om * dT);
            auto sinThOmT = std::sin(th + om * dT);

            x_.x() = x.x() + 1 / (om * om) *
                                 ((v * om + a * om * dT) * sinThOmT +
                                  a * cosThOmT - v * om * sinTh - a * cosTh);
            x_.y() = x.y() + 1 / (om * om) *
                                 ((-v * om - a * om * dT) * cosThOmT +
                                  a * sinThOmT + v * om * cosTh - a * sinTh);
        }

        x_.theta() = th + om * dT;
        x_.v() = v + a * dT;
        x_.a() = a + 0;
        x_.omega() = om + 0;

        // Return transitioned state vector
        return x_;
    }
};

template <typename T> class Measurement : public Kalman::Vector<T, 6> {
  public:
    KALMAN_VECTOR(Measurement, T, 6)

     // X-position
    static constexpr size_t X = 0;
    // Y-Position
    static constexpr size_t Y = 1;
    // Orientation
    static constexpr size_t THETA = 2;
    // X velocity
    static constexpr size_t V = 3;
    // Y velocity
    static constexpr size_t A = 4;
    // Angular velocity
    static constexpr size_t OMEGA = 5;

    T x() const { return (*this)[X]; }
    T y() const { return (*this)[Y]; }
    T theta() const { return (*this)[THETA]; }
    T v() const { return (*this)[V]; }
    T a() const { return (*this)[A]; }
    T omega() const { return (*this)[OMEGA]; }

    T &x() { return (*this)[X]; }
    T &y() { return (*this)[Y]; }
    T &theta() { return (*this)[THETA]; }
    T &v() { return (*this)[V]; }
    T &a() { return (*this)[A]; }
    T &omega() { return (*this)[OMEGA]; }
};

/**
 * @brief Measurement model
 *
 *
 * @param T Numeric scalar type
 * @param CovarianceBase Class template to determine the covariance
 * representation (as covariance matrix (StandardBase) or as lower-triangular
 *                       coveriace square root (SquareRootBase))
 */
template <typename T,
          template <class> class CovarianceBase = Kalman::StandardBase>
class MeasurementModel
    : public Kalman::LinearizedMeasurementModel<State<T>, Measurement<T>,
                                                CovarianceBase> {
  public:
    // State type shortcut definition
    typedef State<T> S;

    // Measurement type shortcut definition
    typedef Measurement<T> M;

    /**
     * @brief Definition of (possibly non-linear) measurement function
     *
     * This function maps the system state to the measurement that is expected
     * to be received from the sensor assuming the system is currently in the
     * estimated state.
     *
     * @param x The system state in current time-step
     * @returns The predicted sensor measurement for the system state
     */
    M h(const S &x) const {
        M measurement;

        measurement.x() = x.x();
        measurement.y() = x.y();   
        measurement.theta() = x.theta();

        return measurement;
    }
};







#endif // KALMAN_UTILS_HPP_