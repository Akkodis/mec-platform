#ifndef PREDICTOR_HPP_
#define PREDICTOR_HPP_

#include "its_utils.hpp"
#include <boost/circular_buffer.hpp>
#include <boost/circular_buffer/space_optimized.hpp>

// #include "kalman_filters/unscented_kalman_filter.h"
#include <gtsam/geometry/Pose2.h>
#include <gtsam/inference/Key.h>
#include <gtsam/nonlinear/GaussNewtonOptimizer.h>
#include <gtsam/nonlinear/LevenbergMarquardtOptimizer.h>
#include <gtsam/nonlinear/Marginals.h>
#include <gtsam/nonlinear/NonlinearFactorGraph.h>
#include <gtsam/nonlinear/Values.h>
#include <gtsam/slam/BetweenFactor.h>

#include "kalman/kalman_utils.hpp"

/**
 * @brief Predictor class.
 */
class Predictor {

  public:
    /**
     * @brief First predictor configuration.
     */
    virtual void configure() = 0;

    /**
     * @brief Reset predictor configuration.
     */
    virtual void reset() = 0;

    /**
     * @brief Predict the next position of a vehicle given the previous
     * information.
     * @param positions Buffer of positions.
     * @param heading Heading value.
     * @param speed Speed value.
     * @param accelerations Accelerations value.
     * @param yaw_rate Yaw_rate value.
     * @param steering_wheel_angle Steering wheel angle value.
     * @param curvature Curvature value.
     * @return its::Position Predicted position.
     */
    virtual its::Position predict(
        const boost::circular_buffer_space_optimized<its::Position> &positions,
        const its::Heading &heading, const its::Speed &speed,
        const its::Acceleration &accelerations, const its::YawRate &yaw_rate,
        const its::SteeringWheelAngle &steering_wheel_angle,
        const its::Curvature &curvature) = 0;

    /**
     * @brief Get the last predicted position
     * @return its::Position const& Last predicted position
     */
    its::Position const &getPrediction() const;

  protected:
    /**
     * @brief Project the next position applying a motion model.
     * @param positions Buffer of positions.
     * @param heading Heading value.
     * @param speed Speed value.
     * @param accelerations Accelerations value.
     * @param yaw_rate Yaw_rate value.
     * @param steering_wheel_angle Steering wheel angle value.
     * @param curvature Curvature value.
     * @return std::array<double, 3> Array with predicted x, y, theta values.
     */
    std::array<double, 3> project(
        const boost::circular_buffer_space_optimized<its::Position> &positions,
        const std::array<double, 2> &curr_pos, const its::Heading &heading,
        const its::Speed &speed, const its::Acceleration &accelerations,
        const its::YawRate &yaw_rate,
        const its::SteeringWheelAngle &steering_wheel_angle,
        const its::Curvature &curvature);

    /**
     * @brief Apply the motion model.
     * @param curr_pos Current x, y, theta position.
     * @param heading Heading value.
     * @param speed Speed value.
     * @param accelerations Accelerations value.
     * @param yaw_rate Yaw_rate value.
     * @param steering_wheel_angle Steering wheel angle value.
     * @param curvature Curvature value.
     * @param T Time interval from last information.
     * @return std::array<double, 3>
     */
    std::array<double, 3>
    applyModel(const std::array<double, 2> &curr_pos,
               const its::Heading &heading, const its::Speed &speed,
               const its::Acceleration &accelerations,
               const its::YawRate &yaw_rate,
               const its::SteeringWheelAngle &steering_wheel_angle,
               const its::Curvature &curvature, double T, bool force);

    /**
     * @brief Set the Reference Position where conversion from WGS to XY is
     * done.
     * @param pos Position where reference has to be set.
     */
    void setReferencePosition(const its::Position &pos);

    /**
     * @brief Convert WGS position (latitude, longitude) into XY cartesian with
     * respect to a reference.
     * @param pos_WGS WGS position.
     * @param reference Reference position.
     * @return std::array<double, 2> Array with XY position.
     */
    std::array<double, 2> getXYPosition(const std::array<double, 2> &pos_WGS,
                                        const std::array<double, 2> &reference);

    /**
     * @brief Convert Cartesian position.
     * @param pos_XY Cartesian XY position.
     * @param reference
     * @return std::array<double, 2>
     */
    std::array<double, 2>
    getWGSPosition(const std::array<double, 2> &pos_XY,
                   const std::array<double, 2> &reference);

    its::Position last_prediction_;
    std::array<double, 2> reference_position_;
    double reference_time_;
    unsigned int n_iterations_;

    bool _debug_ = false;
};

/**
 * @brief Factor Graph-based predictor class.
 */
class FactorGraphPredictor : public Predictor {

  public:
    its::Position predict(
        const boost::circular_buffer_space_optimized<its::Position> &positions,
        const its::Heading &heading, const its::Speed &speed,
        const its::Acceleration &accelerations, const its::YawRate &yaw_rate,
        const its::SteeringWheelAngle &steering_wheel_angle,
        const its::Curvature &curvature) override;

    void configure() override;

    void reset() override;

  private:
    /**
     * @brief Build the entire Factor Graph.
     * @param positions Input positions.
     * @return int Index where add next position.
     */
    int buildFullFactorGraph(
        const boost::circular_buffer_space_optimized<its::Position> &positions);

    /**
     * @brief Update the graph giving the current and the previous position.
     * @param curr Current position.
     * @param prev Previous position.
     * @return int Index where add next position.
     */
    int updateFactorGraph(const its::Position &curr, const its::Position &prev);

    /**
     * @brief Insert new factor in the i-th position.
     * @param i Index where add the factor.
     * @param curr Current position.
     * @param prev Previous position.
     */
    void insertNewFactor(int i, const its::Position &curr,
                         const its::Position &prev);

    gtsam::NonlinearFactorGraph::shared_ptr graph_;
    gtsam::Values initial_estimate_;
    gtsam::noiseModel::Diagonal::shared_ptr prior_noise_;
    gtsam::noiseModel::Diagonal::shared_ptr noise_model_;
    gtsam::GaussNewtonParams parameters_;
};

/**
 * @brief UKF (Unscendent Kalman Filter) Predictor class.
*/
class UKFPredictor : public Predictor {

    its::Position predict(
        const boost::circular_buffer_space_optimized<its::Position> &positions,
        const its::Heading &heading, const its::Speed &speed,
        const its::Acceleration &accelerations, const its::YawRate &yaw_rate,
        const its::SteeringWheelAngle &steering_wheel_angle,
        const its::Curvature &curvature) override;

    void configure() override;
    void reset() override;

  private:
    static const int RESET_ITERATIONS = 0; // Reset is not provided
    std::shared_ptr<Kalman::UnscentedKalmanFilter<State<double>>> ukf_;
    SystemModel<double> sys_;
    MeasurementModel<double> mm_;
    State<double> x_;
};


/**
 * @brief Simple Predictor class.
*/
class SimplePredictor : public Predictor {

    its::Position predict(
        const boost::circular_buffer_space_optimized<its::Position> &positions,
        const its::Heading &heading, const its::Speed &speed,
        const its::Acceleration &accelerations, const its::YawRate &yaw_rate,
        const its::SteeringWheelAngle &steering_wheel_angle,
        const its::Curvature &curvature) override;

    void configure() override;
    void reset() override;
};

/**
 * @brief Predictor factory class
 */
class PredictorFactory {

  public:
    /**
     * @brief List of predictors currently developed.
     */
    enum class PredictorType { FACTOR, UKF, SIMPLE /*, KALMAN */ };

    /**
     * @brief Get the Predictor object
     * @param type Passing the predictor type it is possible to choise which
     * predictor should be used.
     * @return std::shared_ptr<Predictor> Pointer of the chosen predictor.
     */
    std::shared_ptr<Predictor> getPredictor(PredictorType type);
};

#endif   // PREDICTOR_HPP_

// Kalman Fitler implementation with kalman_filters library
// (https://github.com/tysik/kalman_filters.git)
// @brief Kalman Filter-based predictor (empty).
/*
class KalmanFilterPredictor : public Predictor {

    its::Position predict(
        const boost::circular_buffer_space_optimized<its::Position> &positions,
        const its::Heading &heading, const its::Speed &speed,
        const its::Acceleration &accelerations, const its::YawRate &yaw_rate,
        const its::SteeringWheelAngle &steering_wheel_angle,
        const its::Curvature &curvature) override;

    void configure() override;
    void reset() override;

  private:
    static const int RESET_ITERATIONS = 2048;

    std::function<arma::vec(const arma::vec &q, const arma::vec &u)>
        process_function_;
    std::function<arma::vec(const arma::vec &q)> output_function_;

    std::shared_ptr<kf::UnscentedKalmanFilter> kf_;
};

*/