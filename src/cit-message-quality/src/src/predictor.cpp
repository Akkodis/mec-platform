#include "predictor.hpp"

#include "WGS84toCartesian/WGS84toCartesian.hpp"

#include <iterator>

////////////////////////////////////////////////////////////////////////////////////

static const double YAW_RATE_THRESHOLD = 2.0;
static const int MAX_ITERATIONS = 8;

////////////////////////////////////////////////////////////////////////////////////

std::array<double, 2>
Predictor::getXYPosition(const std::array<double, 2> &pos_WGS,
                         const std::array<double, 2> &reference) {

    return wgs84::toCartesian(reference, pos_WGS);
}

std::array<double, 2>
Predictor::getWGSPosition(const std::array<double, 2> &pos_XY,
                          const std::array<double, 2> &reference) {

    return wgs84::fromCartesian(reference, pos_XY);
}

its::Position const &
Predictor::getPrediction() const {

    return this->last_prediction_;
}

////////////////////////////////////////////////////////////////////////////////////

enum class STATE_TRANSITION_MODELS { CV, CTRA };

STATE_TRANSITION_MODELS
checkModels(double speed, double accelerations, double yaw_rate,
            double steering_wheel_angle, double curvature, bool force) {

    // feature for forcing the choise of the model
    if (force)
        return STATE_TRANSITION_MODELS::CTRA;

    // if the yaw rate is to low it is chosen another simpler model
    if (yaw_rate <= YAW_RATE_THRESHOLD) {
        return STATE_TRANSITION_MODELS::CV;
    } else
        return STATE_TRANSITION_MODELS::CTRA;
}

std::array<double, 3>
applyCV(double x, double y, double theta, double v, double T) {

    std::array<double, 3> ret;

    ret[0] = x + T * v;
    ret[1] = y + T * v;
    ret[2] = theta;

    return ret;
}

std::array<double, 3>

applyCTRA(double x, double y, double theta, double v, double a, double w,
          double T) {

    std::array<double, 3> ret;
    double w_ = w;

    double Dx = (1 / (std::pow(w_, 2))) *
                ((v * w_ + a * w_ * T) * std::sin(theta + w_ * T) +
                    a * std::cos(theta + w_ * T) - v * w_ * std::sin(theta) -
                    a * std::cos(theta));

    double Dy = (1 / (std::pow(w_, 2))) *
                ((-v * w_ - a * w_ * T) * std::cos(theta + w_ * T) +
                    a * std::sin(theta + w_ * T) + v * w_ * std::cos(theta) -
                    a * std::cos(theta));

    ret[0] = x + Dx;
    ret[1] = y + Dy;
    ret[2] = theta + w_ * T;
    return ret;

}

std::array<double, 3>
Predictor::applyModel(const std::array<double, 2> &curr_pos,
                      const its::Heading &heading, const its::Speed &speed,
                      const its::Acceleration &accelerations,
                      const its::YawRate &yaw_rate,
                      const its::SteeringWheelAngle &steering_wheel_angle,
                      const its::Curvature &curvature, double T, bool force) {

    std::array<double, 3> ret;
    double x, y;
    double theta = heading.getValue();
    double v = speed.getValue();
    double a = accelerations.getLongitudinal();
    double w = yaw_rate.getValue();
    double phi = steering_wheel_angle.getValue();
    double cur = curvature.getValue();

    x = curr_pos[0];
    y = curr_pos[1];

    switch (checkModels(v, a, w, phi, cur, force)) {

    case STATE_TRANSITION_MODELS::CV:
        ret = applyCV(x, y, theta, v, T);
        break;

    case STATE_TRANSITION_MODELS::CTRA:
        ret = applyCTRA(x, y, theta, v, a, w, T);
        break;
    }

    return ret;
}

std::array<double, 3>
Predictor::project(
    const boost::circular_buffer_space_optimized<its::Position> &positions,
    const std::array<double, 2> &curr_pos, const its::Heading &heading,
    const its::Speed &speed, const its::Acceleration &accelerations,
    const its::YawRate &yaw_rate,
    const its::SteeringWheelAngle &steering_wheel_angle,
    const its::Curvature &curvature) {

    auto last = std::prev(positions.end(), 1);
    double T = (*last).getTime() - this->reference_time_;

    if (last != std::prev(positions.begin(), -1)) {

        auto prev = std::prev(positions.end(), 2);
        T = T - ((*prev).getTime() - this->reference_time_);
    }

    if (T == 0)
        T = its::MEAN_DELTA_TIME;

    return applyModel(curr_pos, heading, speed, accelerations, yaw_rate,
                      steering_wheel_angle, curvature, T, false);
}

void
Predictor::setReferencePosition(const its::Position &pos) {

    this->reference_position_ = pos.getArrayPosition();
    this->reference_time_ = pos.getTime();
}

////////////////////////////////////////////////////////////////////////////////////

void
FactorGraphPredictor::insertNewFactor(int i, const its::Position &curr,
                                      const its::Position &prev) {

    std::array<double, 2> curr_XY =
        getXYPosition(curr.getArrayPosition(), reference_position_);

    if (i == 0) {
        initial_estimate_.insert(i, gtsam::Pose2(curr_XY[0], curr_XY[1], 0));
        return;
    } else
        initial_estimate_.insert(i, gtsam::Pose2(curr_XY[0], curr_XY[1],
                                                 curr.getHeading().getValue()));

    std::array<double, 2> prev_XY =
        getXYPosition(prev.getArrayPosition(), reference_position_);

    gtsam::Pose2 prev_pos = gtsam::Pose2(prev.getHeading().getValue(),
                                         gtsam::Point2(prev_XY[0], prev_XY[1]));

    gtsam::Pose2 curr_pos = gtsam::Pose2(curr.getHeading().getValue(),
                                         gtsam::Point2(curr_XY[0], curr_XY[1]));

    graph_->emplace_shared<gtsam::BetweenFactor<gtsam::Pose2>>(
        i, i - 1, curr_pos.between(prev_pos), noise_model_);

    if (_debug_) {
        std::cout << "//////////////////////////////////////////////////"
                  << std::endl;

        std::cout << "i: " << i << std::endl;
        std::cout << "position prev: " << std::endl;
        std::cout << prev << std::endl;
        std::cout << "position curr: " << std::endl;
        std::cout << curr << std::endl;

        std::cout << "array prev: " << prev_XY[0] << " " << prev_XY[1]
                  << std::endl;
        std::cout << "array curr: " << curr_XY[0] << " " << curr_XY[1]
                  << std::endl;

        std::cout << "prev: " << std::endl;
        prev_pos.print();
        std::cout << "curr: " << std::endl;
        curr_pos.print();

        std::cout << "//////////////////////////////////////////////////"
                  << std::endl;
    }
}

int
FactorGraphPredictor::buildFullFactorGraph(
    const boost::circular_buffer_space_optimized<its::Position> &positions) {

    int i = 0;

    for (auto it = positions.begin(); it != positions.end(); it++, i++) {

        auto prev = std::prev(it, 1);
        insertNewFactor(i, *it, *prev);
    }

    return i;
}

////////////////////////////////////////////////////////////////////////////////////
void
FactorGraphPredictor::configure() {

    this->graph_ = std::make_shared<gtsam::NonlinearFactorGraph>();

    double noise_var = 1e-5;

    this->prior_noise_ = gtsam::noiseModel::Diagonal::Sigmas(
        gtsam::Vector3(noise_var, noise_var, noise_var));

    this->graph_->addPrior(0, gtsam::Pose2(0, 0, 0), this->prior_noise_);

    this->noise_model_ = gtsam::noiseModel::Diagonal::Sigmas(
        gtsam::Vector3(noise_var, noise_var, noise_var));

    // Stop iterating once the change in error between steps is less than this
    // value
    this->parameters_.relativeErrorTol = 1e-5;

    // Do not perform more than N iteration steps
    this->parameters_.maxIterations = 100;
}

void
FactorGraphPredictor::reset() {

    gtsam::NonlinearFactorGraph new_graph;
    new_graph.addPrior(0, gtsam::Pose2(0, 0, 0), this->prior_noise_);

    this->graph_ =
        std::make_shared<gtsam::NonlinearFactorGraph>(std::move(new_graph));
    this->initial_estimate_ = gtsam::Values();
}

int
FactorGraphPredictor::updateFactorGraph(const its::Position &curr,
                                        const its::Position &prev) {

    insertNewFactor(this->n_iterations_, curr, prev);
    return this->n_iterations_ + 1;
}

its::Position
FactorGraphPredictor::predict(
    const boost::circular_buffer_space_optimized<its::Position> &positions,
    const its::Heading &heading, const its::Speed &speed,
    const its::Acceleration &accelerations, const its::YawRate &yaw_rate,
    const its::SteeringWheelAngle &steering_wheel_angle,
    const its::Curvature &curvature) {

    // Not enough position to make any optimizations. The prediction is made
    // using only the model
    if (positions.size() < 2) {

        auto &actual_pos = *(positions.begin());
        auto pos_array =
            applyModel(std::array<double, 2>{0, 0}, heading, speed,
                       accelerations, yaw_rate, steering_wheel_angle, curvature,
                       its::MEAN_DELTA_TIME, false);

        auto pos_WGS =
            getWGSPosition(std::array<double, 2>{pos_array[0], pos_array[1]},
                           std::array<double, 2>{actual_pos.getLatitude(),
                                                 actual_pos.getLongitude()});

        this->last_prediction_ = actual_pos;
        this->last_prediction_.step(pos_WGS[0], pos_WGS[1], pos_array[3]);

        return this->last_prediction_;
    }

    // Prediction is made by applying a motion model corrected by a Factor
    // Graph-based optimization. The graph is updated every 64 iterations.
    int i = 0;
    auto last = std::prev(positions.end(), 1);

    // 1. Factor Graph creation
    // The graph is stored dynamically with a maximum fixed size. Whether
    // iterations goes above this dimention then the graph is reset.
    if (n_iterations_ == 0) {

        // set the reference
        setReferencePosition(*(positions.begin()));
        // build the graph
        i = buildFullFactorGraph(positions);
        // in the graph will be stored 2 positions
        n_iterations_ += 2;

    } else if (n_iterations_ >= MAX_ITERATIONS) {
        // reset the graph, build it again with the positions actually stored
        // into the buffer

        // reset the graph
        reset();

        // set the reference
        setReferencePosition(*(positions.begin()));

        // build the factor graph again
        i = buildFullFactorGraph(positions);

        n_iterations_ = positions.size();

    } else {
        // simply update the graph
        auto prev = std::prev(last, 1);
        i = updateFactorGraph(*last, *prev);

        n_iterations_++;
    }

    // the idea is, after the creation of the pose graph, it is added also the
    // predicted position after the prediction, the last position is arised.

    // in the position "i" will be stored the projection

    // 2. Pose prediction
    // Apply one physical model to predict the pose
    auto curr_pos =
        getXYPosition((*last).getArrayPosition(), this->reference_position_);

    std::array<double, 3> projection =
        project(positions, curr_pos, heading, speed, accelerations, yaw_rate,
                steering_wheel_angle, curvature);

    // 3. Update the Factor Graph with the projection
    this->initial_estimate_.insert(
        i, gtsam::Pose2(projection[0], projection[1], projection[2]));

    gtsam::Pose2 curr_pos2 = gtsam::Pose2(
        heading.getValue(), gtsam::Point2(curr_pos[0], curr_pos[1]));

    gtsam::Pose2 pred_pos = gtsam::Pose2(
        projection[2], gtsam::Point2(projection[0], projection[1]));

    this->graph_->emplace_shared<gtsam::BetweenFactor<gtsam::Pose2>>(
        i, i - 1, pred_pos.between(curr_pos2), this->noise_model_);

    // 4. Optimize the initial values using a Gauss-Newton nonlinear optimizer
    // Create the optimizer ...
    gtsam::LevenbergMarquardtParams params;
    params.linearSolverType = gtsam::NonlinearOptimizerParams::Iterative;
    params.iterativeParams =
        std::make_shared<gtsam::SubgraphSolverParameters>();

    gtsam::LevenbergMarquardtOptimizer optimizer(
        *(this->graph_), this->initial_estimate_, params);
    // ... and optimize
    gtsam::Values result = optimizer.optimize();

    // 5. Unpack the result
    auto &pos_prediction_Pose2 = result.at<gtsam::Pose2>(i);
    auto pos_prediction_WGS =
        getWGSPosition(std::array<double, 2>{pos_prediction_Pose2.x(),
                                             pos_prediction_Pose2.y()},
                       this->reference_position_);

    this->last_prediction_ = *last;
    this->last_prediction_.step(pos_prediction_WGS[0], pos_prediction_WGS[1],
                                projection[2]);

    // 6. Remove the prediction to the graph. This will be replaced with the
    // actual value if the distance metric is below the threshold.
    this->graph_->resize(i);
    this->initial_estimate_.erase(i);

    if (_debug_) {
        std::cout << "Initial: " << std::endl;
        this->initial_estimate_.print();

        std::cout << "Graph: " << std::endl;
        this->graph_->print();

        std::cout << "Result: " << std::endl;
        result.print();

        std::cout << "[predict: projection]: x: " << projection[0]
                  << ", y: " << projection[1] << ", theta: " << projection[2]
                  << std::endl;

        std::cout << "[predict: prediction]: x: " << pos_prediction_Pose2.x()
                  << ", y: " << pos_prediction_Pose2.y()
                  << ", theta: " << pos_prediction_Pose2.theta() << std::endl;
    }

    return this->last_prediction_;
}

////////////////////////////////////////////////////////////////////////////////////

void
UKFPredictor::configure() {

    ukf_ = std::make_shared<Kalman::UnscentedKalmanFilter<State<double>>>(
        Kalman::UnscentedKalmanFilter<State<double>>(0.5, 2.0, 0.0));

    // Kalman::Covariance<State<double>> state_cov;
    // state_cov.setIdentity();
    // state_cov(State<double>::X, State<double>::X) = 0.1;
    // state_cov(State<double>::Y, State<double>::Y) = 0.1;
    // state_cov(State<double>::A, State<double>::A) = 0.1;
    // state_cov(State<double>::V, State<double>::V) = 0.1;
    // state_cov(State<double>::THETA, State<double>::THETA) = 0.1;
    // state_cov(State<double>::OMEGA, State<double>::OMEGA) = 0.1;
    // ukf_->setCovariance(state_cov);

    // // Set process noise covariance
    // Kalman::Covariance<State<double>> cov;
    // cov.setIdentity();
    // cov(State<double>::X, State<double>::X) = 0.1;
    // cov(State<double>::Y, State<double>::Y) = 0.1;
    // cov(State<double>::A, State<double>::A) = 0.1;
    // cov(State<double>::V, State<double>::V) = 0.1;
    // cov(State<double>::THETA, State<double>::THETA) = 0.2;
    // cov(State<double>::OMEGA, State<double>::OMEGA) = 0.2;
    // sys_.setCovariance(cov);

    // Kalman::Covariance<Measurement<double>> cov_m;
    // cov_m.setIdentity();
    // cov_m(Measurement<double>::X, Measurement<double>::X) = 1.0;
    // cov_m(Measurement<double>::Y, Measurement<double>::Y) = 1.0;
    // cov_m(Measurement<double>::THETA, Measurement<double>::THETA) = 1.0;
    // mm_.setCovariance(cov_m);

    x_.setZero();
    ukf_->init(x_);
}

its::Position
UKFPredictor::predict(
    const boost::circular_buffer_space_optimized<its::Position> &positions,
    const its::Heading &heading, const its::Speed &speed,
    const its::Acceleration &accelerations, const its::YawRate &yaw_rate,
    const its::SteeringWheelAngle &steering_wheel_angle,
    const its::Curvature &curvature) {

    State<double> x;

    if (this->n_iterations_ == 0) {
        setReferencePosition(*(positions.begin()));
    }

    if (this->n_iterations_ >= RESET_ITERATIONS) {
        setReferencePosition(*(positions.begin()));
        this->reset();
    }

    this->n_iterations_++;

    auto last = std::prev(positions.end(), 1);
    auto curr_pos =
        getXYPosition((*last).getArrayPosition(), this->reference_position_);

    double x_curr, y_curr;
    double theta = heading.getValue();
    double v = speed.getValue();
    double a = accelerations.getLongitudinal();
    double w = yaw_rate.getValue();
    double phi = steering_wheel_angle.getValue();
    double cur = curvature.getValue();

    x_curr = curr_pos[0];
    y_curr = curr_pos[1];

    if (_debug_) {
        std::cout << "Iteration: " << this->n_iterations_ << std::endl;
        std::cout << "[Curr_pos]: x: " << x_curr << " | y: " << y_curr
                  << std::endl;
    }

    double T = (*last).getTime() - this->reference_time_;

    if (last != std::prev(positions.begin(), -1)) {

        auto prev = std::prev(positions.end(), 2);
        T = T - ((*prev).getTime() - this->reference_time_);
    }

    if (T == 0)
        T = its::MEAN_DELTA_TIME;

    Control<double> u;
    u.dt() = T;
    // here x_ must be filled with the x,y,...
    x_.x() = x_curr;
    x_.y() = y_curr;
    x_.theta() = theta;
    x_.v() = v;
    x_.a() = a;
    x_.w() = w;

    // auto x_pred = sys_.f(x_, u);

    // auto x_pred = sys_.f(x_, u);
    auto x_ukf = ukf_->predict(sys_, u);
    Measurement<double> mm = mm_.h(x_);
    x_ukf = ukf_->update(mm_, mm);

    if (_debug_)
        std::cout << "[Prediction]: x: " << x_ukf.x() << " | y: " << x_ukf.y()
                  << " | theta: " << x_ukf.theta() << std::endl;

    auto pos_prediction_WGS = getWGSPosition(
        std::array<double, 2>{x_ukf.x(), x_ukf.y()}, this->reference_position_);

    this->last_prediction_ = *last;
    this->last_prediction_.step(pos_prediction_WGS[0], pos_prediction_WGS[1],
                                x_ukf.theta());

    if (_debug_)
        std::cout << "[Prediction]: lat: " << pos_prediction_WGS[0]
                  << " | long: " << pos_prediction_WGS[1] << std::endl;

    return this->last_prediction_;
}

void
UKFPredictor::reset() {
    std::cout << "not provided (until now)" << std::endl;
};

////////////////////////////////////////////////////////////////////////////////////

its::Position
SimplePredictor::predict(
    const boost::circular_buffer_space_optimized<its::Position> &positions,
    const its::Heading &heading, const its::Speed &speed,
    const its::Acceleration &accelerations, const its::YawRate &yaw_rate,
    const its::SteeringWheelAngle &steering_wheel_angle,
    const its::Curvature &curvature) {

    if (this->n_iterations_++ == 0) {
        setReferencePosition(*(positions.begin()));
    }

    auto last = std::prev(positions.end(), 1);

    auto curr_pos =
        getXYPosition((*last).getArrayPosition(), this->reference_position_);

    std::array<double, 3> projection =
        project(positions, curr_pos, heading, speed, accelerations, yaw_rate,
                steering_wheel_angle, curvature);

    auto pos_prediction_WGS =
        getWGSPosition(std::array<double, 2>{projection[0], projection[1]},
                       this->reference_position_);

    this->last_prediction_ = *last;
    this->last_prediction_.step(pos_prediction_WGS[0], pos_prediction_WGS[1],
                                projection[2]);

    return this->last_prediction_;
}

void
SimplePredictor::configure() {}

void
SimplePredictor::reset() {}

////////////////////////////////////////////////////////////////////////////////////

std::shared_ptr<Predictor>
PredictorFactory::getPredictor(PredictorType type) {

    switch (type) {

    case PredictorType::FACTOR:
        return std::make_shared<FactorGraphPredictor>();
    case PredictorType::UKF:
        return std::make_shared<UKFPredictor>();
    case PredictorType::SIMPLE:
        return std::make_shared<SimplePredictor>();
    // case PredictorType::KALMAN:
    //     return std::make_shared<KalmanFilterPredictor>();
    default:
        return nullptr;
    }
}

/* Kalman Fitler implementation with kalman_filters library
(https://github.com/tysik/kalman_filters.git)

void
KalmanFilterPredictor::configure() {

    // this->kf = kf::UnscentedKalmanFilter(3, 3, 7);
    this->kf_ = std::make_shared<kf::UnscentedKalmanFilter>(
        kf::UnscentedKalmanFilter(1, 6, 6));

    this->process_function_ = [](arma::vec q, arma::vec u) -> arma::vec {
        float dt = u(0);

        std::cout << "[q]: dt: " << dt << "| x: " << q(0) << " | y: " << q(1)
                  << " | theta: " << q(2) << " | v: " << q(3)
                  << " | a: " << q(4) << " | w: " << q(5) << std::endl;

        if (std::abs(q(5)) < 2.0) {
            if (q(5) < 0) {
                q(5) = -2.0;
            } else
                q(5) = 2.0;
            // std::cout << "[q]: x: " << q(0) << " | y: " << q(1)
            //           << " | theta: " << q(2) << " | v: " << q(3)
            //           << " | a: " << q(4) << " | w: " << q(5) << std::endl;

            // return {q(0) + dt * q(3), q(1) + dt * q(3), q(2), q(3), q(4),
            // q(5)};

            // return {q(0) + dt * q(3), q(1) + dt * q(3), q(2), q(3), q(4)};
        }

        // else {
        return {
            // q(0) +
            //     (q(3) / q(5)) * (std::sin(q(5) * dt + q(2)) -
            //     std::sin(q(2))),
            // q(1) +
            //     (q(3) / q(5)) * (-std::cos(q(5) * dt + q(2)) +
            //     std::sin(q(2))),
            // q(2) + q(5) * dt,
            // q(3),
            // q(4),
            // q(5)

            q(0) + (1 / (std::pow(q(5), 2))) *
                       ((q(3) * q(5) + q(4) * q(5) * dt) *
                            std::sin(q(2) + q(5) * dt) +
                        q(4) * std::cos(q(2) + q(5) * dt) -
                        q(3) * q(5) * std::sin(q(2)) - q(4) * std::cos(q(2))),
            q(1) + (1 / (std::pow(q(5), 2))) *
                       ((-q(3) * q(5) - q(4) * q(5) * dt) *
                            std::cos(q(2) + q(5) * dt) +
                        q(4) * std::sin(q(2) + q(5) * dt) +
                        q(3) * q(5) * std::cos(q(2)) - q(4) * std::sin(q(2))),
            q(2) + q(5) * dt,
            q(3) + q(4) * dt,
            q(4),
            q(5)

            //             q(0) + (1 / (std::pow(q(5), 2))) *
            //             ((q(3) * q(5) + q(4) * q(5) * dt) *
            //                 std::sin(q(2) + q(5) * dt) +
            //             q(4) * std::cos(q(2) + q(5) * dt) -
            //             q(3) * q(5) * std::sin(q(2)) - q(4) *
            //             std::cos(q(2))),
            //             q(1) + (1 / (std::pow(q(5), 2))) *
            //                        ((-q(3) * q(5) - q(4) * q(5) * dt) *
            //                             std::cos(q(2) + q(5) * dt) +
            //                         q(4) * std::sin(q(2) + q(5) * dt) +
            //                         q(3) * q(5) * std::cos(q(2)) - q(4) *
            //                         std::sin(q(2))),
            //             q(2) + q(5) * dt,
            //             q(3) + q(4) * dt,
            //             q(4)
            //    //         q(5)

        };
    };
    // };

    this->output_function_ = [](arma::vec q) -> arma::vec {
        return {q(0), q(1), q(2), q(3), q(4), q(5)};
        // return {q(0), q(1), q(2), q(3), q(4)};
    };

    this->kf_->setProcessFunction(this->process_function_);
    this->kf_->setOutputFunction(this->output_function_);
    this->kf_->setDesignParameters(0.05, 2.0, 0.0);

    double cov_q = 0.002;
    // matrix nxn, n=state
    // arma::mat Q = {{cov_q, cov_q, 0.1, 0.05, 0.0, 2.0},   //  ///
    //                {0.1, cov_q, 0.3, 0.1, 0.2, 2.0},      // ///
    //                {0.2, 0.1, cov_q, 0.2, 0.1, 2.0},      // ///
    //                {0.1, 0.0, 0.0, cov_q, 0.0, 2.0},      // ///
    //                {0.0, 0.0, 0.0, 0.0, cov_q, 2.0},      //  ///
    //                {0.0, 0.0, 0.0, 0.0, 0.0, 2.0}};       // ////

    // arma::mat Q = {{0.001, 0.01, 0.0, 0.0, 0.0, 0.0},   //  ///
    //                {0.01, 0.001, 0.0, 0.0, 0.0, 0.0},     // ///
    //                {0.0, 0.0, 0.01, 0.0, 0.0, 0.0},       // ///
    //                {0.0, 0.0, 0.0, 0.01, 0.0, 0.0},       // ///
    //                {0.0, 0.0, 0.0, 0.0, 0.01, 0.0},       //  ///
    //                {0.0, 0.0, 0.0, 0.0, 0.0, 0.01}};    // ////

    // arma::mat Q = {{cov_q, 0.0, 0.001, 0.0, 0.0, 0.001},        //  ///
    //                {0.0, cov_q, 0.001, 0.0, 0.0, 0.001},        // ///
    //                {0.001, 0.001, cov_q, 0.0, 0.0, 0.001},        // ///
    //                {0.0, 0.0, 0.0, cov_q, 0.0, 0.001},            // ///
    //                {0.0, 0.0, 0.0, 0.0, cov_q, 0.001},            //  ///
    //                {0.001, 0.001, 0.001, 0.001, 0.001, cov_q}};   // ////

    // double x_cov = 1e-17;
    // double y_cov = 1e-17;
    // double theta_cov = 1e-10;
    // double v_cov = 1e-10;
    // double a_cov = 1e-12;
    // double w_cov = 1e-10;

    double x_cov = 0.01;
    double y_cov = 0.01;
    double theta_cov = 0.01;
    double v_cov = 0.01;
    double a_cov = 0.01;
    double w_cov = 0.01;

    arma::mat Q = {{x_cov, 0.0, theta_cov, v_cov, a_cov, w_cov},         //  ///
                   {0.0, y_cov, theta_cov, v_cov, a_cov, w_cov},         // ///
                   {theta_cov, theta_cov, theta_cov, 0.0, 0.0, w_cov},   // ///
                   {v_cov, v_cov, 0.0, v_cov, a_cov, 0.0},               // ///
                   {a_cov, a_cov, 0.0, a_cov, a_cov, 0.0},               //  ///
                   {w_cov, w_cov, w_cov, 0.0, 0.0, w_cov}};              // ////

    // arma::mat Q = {{cov_q, cov_q, 0.0, 0.0, 0.0},   //  ///
    //                {cov_q, cov_q, 0.0, 0.0, 0.0},     // ///
    //                {0.0, 0.0, cov_q, 0.0, 0.0},       // ///
    //                {0.0, 0.0, 0.0, cov_q, 0.0},       // ///
    //                {0.0, 0.0, 0.0, 0.0, cov_q}};       //  ///

    // matrix mxm, m=out
    double cov_r = 0.1;
    // arma::mat R = {{cov_r, 0.0}, {0.0, cov_r}};

    arma::mat R = {{cov_r, 0.0, 0.0, 0.0, 0.0, 0.0},   //  ///
                   {0.0, cov_r, 0.0, 0.0, 0.0, 0.0},   // ///
                   {0.0, 0.0, cov_r, 0.0, 0.0, 0.0},   // ///
                   {0.0, 0.0, 0.0, cov_r, 0.0, 0.0},   // ///
                   {0.0, 0.0, 0.0, 0.0, cov_r, 0.0},   //  ///
                   {0.0, 0.0, 0.0, 0.0, 0.0, cov_r}};

    // arma::mat R = {{cov_r, 0.0, 0.0, 0.0, 0.0},   //  ///
    //                {0.0, cov_r, 0.0, 0.0, 0.0},   // ///
    //                {0.0, 0.0, cov_r, 0.0, 0.0},   // ///
    //                {0.0, 0.0, 0.0, cov_r, 0.0},   // ///
    //                {0.0, 0.0, 0.0, 0.0, cov_r}};   //  ///

    this->kf_->setProcessCovariance(Q);
    this->kf_->setOutputCovariance(R);

    this->n_iterations_ = 0;

    return;
}

its::Position
KalmanFilterPredictor::predict(
    const boost::circular_buffer_space_optimized<its::Position> &positions,
    const its::Heading &heading, const its::Speed &speed,
    const its::Acceleration &accelerations, const its::YawRate &yaw_rate,
    const its::SteeringWheelAngle &steering_wheel_angle,
    const its::Curvature &curvature) {

    if (this->n_iterations_ == 0)
        setReferencePosition(*(positions.begin()));

    if (this->n_iterations_ >= RESET_ITERATIONS) {
        setReferencePosition(*(positions.begin()));
        this->reset();
    }

    this->n_iterations_++;

    auto last = std::prev(positions.end(), 1);
    auto curr_pos =
        getXYPosition((*last).getArrayPosition(), this->reference_position_);

    double x, y;
    double theta = heading.getValue();
    double v = speed.getValue();
    double a = accelerations.getLongitudinal();
    double w = yaw_rate.getValue();
    double phi = steering_wheel_angle.getValue();
    double cur = curvature.getValue();

    x = curr_pos[0];
    y = curr_pos[1];

    std::cout << "Iteration: " << this->n_iterations_ << std::endl;
    std::cout << "[Curr_pos]: x: " << x << " | y: " << y << std::endl;

    // std::array<double, 3> projection =
    //     applyModel(curr_pos, heading, speed, accelerations, yaw_rate,
    //                steering_wheel_angle, curvature, 0.2, true);

    double T = (*last).getTime() - this->reference_time_;

    if (last != std::prev(positions.begin(), -1)) {

        auto prev = std::prev(positions.end(), 2);
        T = T - ((*prev).getTime() - this->reference_time_);
    }

    if (T == 0)
        T = its::MEAN_DELTA_TIME;

    auto projection = this->process_function_({x, y, theta, v, a, w}, {T});

    auto y_ =
        this->output_function_({projection(0), projection(1), projection(2),
                                projection(3), projection(4), projection(5)});

    this->kf_->updateState({T}, y_);
    // this->kf_->updateState({T}, y_);

    auto result = this->kf_->getEstimate();

    std::cout << "[Projection]: x: " << projection(0)
              << " | y: " << projection(1) << " | theta: " << projection(2)
              << std::endl;

    std::cout << "[Prediction]: x: " << result(0) << " | y: " << result(1)
              << " | theta: " << result(2) << std::endl;

    auto pos_prediction_WGS = getWGSPosition(
        std::array<double, 2>{result(0), result(1)}, this->reference_position_);

    auto pos_projection_WGS =
        getWGSPosition(std::array<double, 2>{projection(0), projection(1)},
                       this->reference_position_);

    this->last_prediction_ = *last;
    this->last_prediction_.step(pos_prediction_WGS[0], pos_prediction_WGS[1],
                                projection(2));

    std::cout << "[Projection]: lat: " << pos_projection_WGS[0]
              << " | long: " << pos_projection_WGS[1] << std::endl;

    std::cout << "[Prediction]: lat: " << pos_prediction_WGS[0]
              << " | long: " << pos_prediction_WGS[1] << std::endl;

    return this->last_prediction_;
}

void
KalmanFilterPredictor::reset() {

    this->kf_.reset();
    this->configure();

    return;
}
*/