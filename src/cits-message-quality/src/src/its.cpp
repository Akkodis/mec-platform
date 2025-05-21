#include "its.hpp"

namespace its {

////////////////////////////////////////////////////////////////////////////////////

// Default constructor.
ITS::ITS() noexcept : positions_{MAX_POSITIONS_} {}

////////////////////////////////////////////////////////////////////////////////////

// Constuctor with json message.
ITS::ITS(const rapidjson::Document &cam_message) noexcept
    : positions_(MAX_POSITIONS_) {

    const rapidjson::Value &cam_basic_container =
        cam_message["cam"]["camParameters"]["basicContainer"];
    const rapidjson::Value &cam_high_container =
        cam_message["cam"]["camParameters"]["highFrequencyContainer"]
                   ["basicVehicleContainerHighFrequency"];

    this->station_id_ = cam_message["header"]["stationID"].GetInt();

    this->current_generation_delta_time_ =
        cam_message["cam"]["generationDeltaTime"].GetInt();

    this->station_type_ = StationClassificationType{
        cam_message["cam"]["camParameters"]["basicContainer"]["stationType"]
            .GetInt()};

    this->heading_ = Heading(cam_high_container);

    this->last_position_ =
        Position{cam_basic_container, this->current_generation_delta_time_,
                 this->heading_};

    this->positions_.push_back(this->last_position_);
    this->speed_ = Speed(cam_high_container);
    this->accelerations_ = Acceleration(cam_high_container);
    this->curvature_ = Curvature(cam_high_container);
    this->yaw_rate_ = YawRate(cam_high_container);
    this->steering_wheel_angle_ = SteeringWheelAngle(cam_high_container);

    if (cam_high_container.HasMember("driveDirection"))
        this->drive_direction_ =
            cam_high_container["driveDirection"].GetString();

    if (cam_high_container.HasMember("curvatureCalculationMode"))
        this->curvature_calculation_mode_ =
            cam_high_container["curvatureCalculationMode"].GetString();

    PredictorFactory pred_factor;
    this->predictor_ =
        pred_factor.getPredictor(PredictorFactory::PredictorType::SIMPLE);
    this->predictor_->configure();

    if (_debug_)
        std::cout << *this << std::endl;
}

////////////////////////////////////////////////////////////////////////////////////

// Constuctor with json message emply.
ITS::ITS(const rapidjson::Document &cam_message, bool empty) noexcept {

    const rapidjson::Value &cam_basic_container =
        cam_message["cam"]["camParameters"]["basicContainer"];
    const rapidjson::Value &cam_high_container =
        cam_message["cam"]["camParameters"]["highFrequencyContainer"]
                   ["basicVehicleContainerHighFrequency"];

    this->station_id_ = cam_message["header"]["stationID"].GetInt();

    this->current_generation_delta_time_ =
        cam_message["cam"]["generationDeltaTime"].GetUint();

    this->station_type_ = StationClassificationType{
        cam_message["cam"]["camParameters"]["basicContainer"]["stationType"]
            .GetInt()};

    this->heading_ = Heading(cam_high_container);

    this->last_position_ =
        Position{cam_basic_container, this->current_generation_delta_time_,
                 this->heading_};

    this->speed_ = Speed(cam_high_container);
    this->accelerations_ = Acceleration(cam_high_container);
    this->curvature_ = Curvature(cam_high_container);
    this->yaw_rate_ = YawRate(cam_high_container);
    this->steering_wheel_angle_ = SteeringWheelAngle(cam_high_container);

    if (cam_high_container.HasMember("driveDirection"))
        this->drive_direction_ =
            cam_high_container["driveDirection"].GetString();

    if (cam_high_container.HasMember("curvatureCalculationMode"))
        this->curvature_calculation_mode_ =
            cam_high_container["curvatureCalculationMode"].GetString();
}

////////////////////////////////////////////////////////////////////////////////////

// Move constructor
ITS::ITS(ITS &&other) noexcept
    : station_id_(std::move(other.station_id_)),
      current_generation_delta_time_(
          std::move(other.current_generation_delta_time_)),
      station_type_(std::move(other.station_type_)),
      positions_(std::move(other.positions_)),
      last_position_(std::move(other.last_position_)),
      drive_direction_(std::move(other.drive_direction_)),
      heading_(std::move(other.heading_)), speed_(std::move(other.speed_)),
      curvature_calculation_mode_(std::move(other.curvature_calculation_mode_)),
      curvature_(std::move(other.curvature_)),
      yaw_rate_(std::move(other.yaw_rate_)),
      steering_wheel_angle_(std::move(other.steering_wheel_angle_)),
      accelerations_(std::move(other.accelerations_)),
      predictor_(std::move(other.predictor_)), _debug_(other._debug_) {}

////////////////////////////////////////////////////////////////////////////////////

// Move assignment operator
ITS &
ITS::operator=(ITS &&other) noexcept {

    if (this != &other) {
        station_id_ = std::move(other.station_id_);
        current_generation_delta_time_ =
            std::move(other.current_generation_delta_time_);
        station_type_ = std::move(other.station_type_);
        positions_ = std::move(other.positions_);
        last_position_ = std::move(other.last_position_);
        drive_direction_ = std::move(other.drive_direction_);
        heading_ = std::move(other.heading_);
        speed_ = std::move(other.speed_);
        curvature_calculation_mode_ =
            std::move(other.curvature_calculation_mode_);
        curvature_ = std::move(other.curvature_);
        yaw_rate_ = std::move(other.yaw_rate_);
        steering_wheel_angle_ = std::move(other.steering_wheel_angle_);
        accelerations_ = std::move(other.accelerations_);
        predictor_ = std::move(other.predictor_);
        _debug_ = other._debug_;
    }

    return *this;
}

////////////////////////////////////////////////////////////////////////////////////

// Copy constructor
ITS::ITS(const ITS &other) noexcept
    : station_id_(other.station_id_),
      current_generation_delta_time_(other.current_generation_delta_time_),
      station_type_(other.station_type_), positions_(other.positions_),
      last_position_(other.last_position_),
      drive_direction_(other.drive_direction_), heading_(other.heading_),
      speed_(other.speed_), yaw_rate_(other.yaw_rate_),
      steering_wheel_angle_(other.steering_wheel_angle_),
      curvature_(other.curvature_),
      curvature_calculation_mode_(other.curvature_calculation_mode_),
      accelerations_(other.accelerations_), predictor_(other.predictor_),
      _debug_(other._debug_) {}

////////////////////////////////////////////////////////////////////////////////////

// Copy assignemnt operator
ITS &
ITS::operator=(const ITS &other) {

    if (this != &other) {
        station_id_ = other.station_id_;
        current_generation_delta_time_ = other.current_generation_delta_time_;
        station_type_ = other.station_type_;
        positions_ = other.positions_;
        last_position_ = other.last_position_;
        drive_direction_ = other.drive_direction_;
        heading_ = other.heading_;
        speed_ = other.speed_;
        yaw_rate_ = other.yaw_rate_;
        steering_wheel_angle_ = other.steering_wheel_angle_;
        curvature_ = other.curvature_;
        curvature_calculation_mode_ = other.curvature_calculation_mode_;
        accelerations_ = other.accelerations_;
        predictor_ = other.predictor_;
        _debug_ = other._debug_;
    }

    return *this;
}

////////////////////////////////////////////////////////////////////////////////////

int
ITS::sizePositions() {
    return positions_.size();
}

////////////////////////////////////////////////////////////////////////////////////

int
ITS::update(const its::ITS &its) {

    if (this->station_id_ != its.getStationId()) {
        std::cout << "ITS::Update: [ERROR]: different station identifiers"
                  << std::endl;
        return -1;
    }

    this->positions_.push_back(its.last_position_);
    this->last_position_ = std::move(its.last_position_);
    this->drive_direction_ = std::move(its.drive_direction_);
    this->heading_ = std::move(its.heading_);
    this->speed_ = std::move(its.speed_);
    this->yaw_rate_ = std::move(its.yaw_rate_);
    this->steering_wheel_angle_ = std::move(its.steering_wheel_angle_);
    this->curvature_ = std::move(its.curvature_);
    this->accelerations_ = std::move(its.accelerations_);

    return 0;
}

////////////////////////////////////////////////////////////////////////////////////

void
ITS::updateWithPrediction() {

    const its::Position &prediction = this->predictor_->getPrediction();

    this->positions_.push_back(prediction);
    this->last_position_ = prediction;
    this->heading_ = Heading(prediction.getHeading().getValue(),
                             prediction.getHeading().getConfidence());
}

////////////////////////////////////////////////////////////////////////////////////

Position
ITS::predictNextPosition() {

    return predictor_->predict(this->positions_, this->heading_, this->speed_,
                               this->accelerations_, this->yaw_rate_,
                               this->steering_wheel_angle_, this->curvature_);
}

////////////////////////////////////////////////////////////////////////////////////

inline unsigned int const
ITS::getStationId() const {

    return this->station_id_;
}

////////////////////////////////////////////////////////////////////////////////////

Position const &
ITS::getCurrentPosition() const {

    return this->last_position_;
}

////////////////////////////////////////////////////////////////////////////////////

// << operator overload
std::ostream &
operator<<(std::ostream &output, const ITS &its) {

    output << "----------------------------------------------" << std::endl;
    output << "ITS::ITS: station_id: " << its.station_id_ << std::endl
           << "ITS::ITS: station_type: " << (int) its.station_type_ << std::endl
           << "ITS::ITS: " << its.heading_ << std::endl
           << "ITS::ITS: " << its.speed_ << std::endl
           << "ITS::ITS: " << its.curvature_ << std::endl
           << "ITS::ITS: " << its.yaw_rate_ << std::endl
           << "ITS::ITS: " << its.steering_wheel_angle_ << std::endl
           << "ITS::ITS: drive_direction: " << its.drive_direction_ << std::endl
           << "ITS::ITS: curvature_calculation_mode: "
           << its.curvature_calculation_mode_ << std::endl
           << "ITS::ITS: " << its.accelerations_ << std::endl;

    output << "//////////////////////////////////////////////" << std::endl;
    output << "ITS::ITS: positions path vector. " << std::endl;

    for (auto its_i = its.positions_.begin(); its_i != its.positions_.end();
         ++its_i)
        output << *(its_i) << std::endl;

    output << "//////////////////////////////////////////////" << std::endl;
    output << "----------------------------------------------";

    return output;
}

////////////////////////////////////////////////////////////////////////////////////

}   // namespace its