#include "its_utils.hpp"
#include "rapidjson/filereadstream.h"
#include <cmath>

namespace its {

////////////////////////////////////////////////////////////////////////////////////

// Conversions based on
// https://www.etsi.org/deliver/etsi_ts/102800_102899/10289402/01.02.01_60/ts_10289402v010201p.pdf

static const double ALTITUDE_UNIT_CONVERSION = 0.01;
static const double LONGITUDE_UNIT_CONVERSION = 0.1 * 1e-6;
static const double LATITUDE_UNIT_CONVERSION = 0.1 * 1e-6;
static const double ACCELERATION_UNIT_CONVERSION = 0.1;
static const double HEADING_UNIT_CONVERSION = 0.1 * M_PI / 180;
static const double SPEED_UNIT_CONVERSION = 0.01;
static const double YAW_RATE_UNIT_CONVERSION = 0.01;
static const double STEERING_WHEEL_ANGLE_CONVERSION = 1.5;
static const double TIME_CONVERSION = 0.001;

////////////////////////////////////////////////////////////////////////////////////

// Support function used mainly in testing
rapidjson::Document
getDocument(const std::string &path) {

    rapidjson::Document d;

    char readBuffer[65536];

    FILE *fp = fopen(path.c_str(), "rb");
    rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));
    d.ParseStream(is);

    fclose(fp);

    return d;
}

////////////////////////////////////////////////////////////////////////////////////

// Constructor with values
PositionConfidence::PositionConfidence(int semi_major, int semi_minor,
                                       int orientation) {

    semi_major_confidence_ = semi_major;
    semi_minor_confidence_ = semi_minor;
    semi_minor_orientation_ = orientation;
}

// Default constructor
PositionConfidence::PositionConfidence()
    : semi_major_confidence_{0}, semi_minor_confidence_{0},
      semi_minor_orientation_{0} {}

// << operator overload
std::ostream &
operator<<(std::ostream &output, const PositionConfidence &pos) {

    output << "{ semi_major_confidence: " << pos.semi_major_confidence_
           << " || semi_minor_confidence: " << pos.semi_minor_confidence_
           << " || semi_minor_orientation: " << pos.semi_minor_orientation_
           << " }";

    return output;
}

////////////////////////////////////////////////////////////////////////////////////

// Default constructor
Position::Position()
    : altitude_{std::make_pair<int, std::string>(0, "")}, delta_time_{0},
      latitude_{0}, longitude_{0}, theta_{}, confidence_ellipse_{} {}

// Constructor with json message
Position::Position(const rapidjson::Value &message, unsigned int delta_time,
                   const Heading &heading)
    : delta_time_{delta_time}, theta_{heading} {

    // Before getting access to the json member, it is performed a check.

    this->altitude_ = std::make_pair<int, std::string>(0, "unavailable");
    this->confidence_ellipse_ = PositionConfidence();
    this->longitude_ = 0;
    this->latitude_ = 0;

    if (message.HasMember("referencePosition")) {
        const rapidjson::Value &position = message["referencePosition"];

        if (position.HasMember("latitude"))
            this->latitude_ = position["latitude"].GetInt();
        if (position.HasMember("longitude"))
            this->longitude_ = position["longitude"].GetInt();

        if (position.HasMember("altitude")) {
            const rapidjson::Value &altitude = position["altitude"];
            this->altitude_.first = altitude["altitudeValue"].GetInt();
            this->altitude_.second = altitude["altitudeConfidence"].GetString();
        }

        if (position.HasMember("positionConfidenceEllipse")) {
            int s_M_c = 0, s_m_c = 0, s_M_o = 0;
            const rapidjson::Value &confidence =
                position["positionConfidenceEllipse"];

            if (confidence.HasMember("semiMajorConfidence"))
                s_M_c = confidence["semiMajorConfidence"].GetInt();
            if (confidence.HasMember("semiMinorConfidence"))
                s_m_c = confidence["semiMinorConfidence"].GetInt();
            if (confidence.HasMember("semiMajorOrientation"))
                s_M_o = confidence["semiMajorOrientation"].GetInt();

            this->confidence_ellipse_ = PositionConfidence(s_M_c, s_m_c, s_M_o);
        }
    }
}

// Constructor with values
Position::Position(unsigned int delta_time, int latitude, int longitude,
                   const std::pair<int, int> &heading,
                   std::pair<int, std::string> &&altitude,
                   PositionConfidence &&confidence_ellipse) {

    this->latitude_ = latitude;
    this->longitude_ = longitude;
    this->theta_ = Heading(heading.first, heading.second);
    this->delta_time_ = delta_time;
    this->confidence_ellipse_ = confidence_ellipse;
    this->altitude_ = altitude;
}

// Constructor with conversions from double to int
Position::Position(double latitude, double longitude)
    : delta_time_{0},
      altitude_{std::make_pair<int, std::string>(0, "unavailable")},
      confidence_ellipse_{}, theta_{} {

    this->latitude_ = static_cast<int>(latitude / LATITUDE_UNIT_CONVERSION);
    this->longitude_ = static_cast<int>(longitude / LONGITUDE_UNIT_CONVERSION);
}

// Copy constructor
Position::Position(const Position &other)
    : delta_time_(other.delta_time_), latitude_(other.latitude_),
      longitude_(other.longitude_), theta_(other.theta_),
      altitude_(other.altitude_),
      confidence_ellipse_(other.confidence_ellipse_) {}

// Copy assignment operator
Position &
Position::operator=(const Position &other) {

    if (this != &other) {
        delta_time_ = other.delta_time_;
        latitude_ = other.latitude_;
        longitude_ = other.longitude_;
        theta_ = other.theta_;
        altitude_ = other.altitude_;
        confidence_ellipse_ = other.confidence_ellipse_;
    }
    return *this;
}

// Move constructor definition
Position::Position(Position &&other)
    : delta_time_(std::move(other.delta_time_)),
      latitude_(std::move(other.latitude_)),
      longitude_(std::move(other.longitude_)), theta_(std::move(other.theta_)),
      altitude_(std::move(other.altitude_)),
      confidence_ellipse_(std::move(other.confidence_ellipse_)) {}

// Move assignment operator definition
Position &
Position::operator=(Position &&other) {

    if (this != &other) {
        delta_time_ = std::move(other.delta_time_);
        latitude_ = std::move(other.latitude_);
        longitude_ = std::move(other.longitude_);
        theta_ = std::move(other.theta_);
        altitude_ = std::move(other.altitude_);
        confidence_ellipse_ = std::move(other.confidence_ellipse_);
    }
    return *this;
}

// Calculate the Haversine Distance from this pose and the target
double
Position::calculateDistance(const Position &other) {

    double dist = 0;
    const double R = 6371e3;                // metres
    const double phi1 =
        this->getLatitude() * M_PI / 180;   // phi, lambda in radians
    const double phi2 = other.getLatitude() * M_PI / 180;
    const double Dphi =
        (other.getLatitude() - this->getLatitude()) * M_PI / 180;
    const double Dlambda =
        (other.getLongitude() - this->getLongitude()) * M_PI / 180;

    const double a = std::sin(Dphi / 2) * std::sin(Dphi / 2) +
                     std::cos(phi1) * std::cos(phi2) * std::sin(Dlambda / 2) *
                         std::sin(Dlambda / 2);
    const double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1 - a));

    dist = R * c;   // in metres

    return dist;
}

std::array<double, 2> const
Position::getArrayPosition() const {

    return std::array<double, 2>{{this->getLatitude(), this->getLongitude()}};
}

void
Position::step(double latitude, double longitude, double heading) {

    this->latitude_ = static_cast<int>(latitude / LATITUDE_UNIT_CONVERSION);
    this->longitude_ = static_cast<int>(longitude / LONGITUDE_UNIT_CONVERSION);
    this->theta_ =
        Heading(static_cast<int>(heading / HEADING_UNIT_CONVERSION), 0);
    this->delta_time_ += MEAN_DELTA_TIME;
}

// << operator overload
std::ostream &
operator<<(std::ostream &output, const Position &pos) {

    output << "[" << pos.delta_time_
           << "]: { latitude : " << std::setprecision(10) << pos.getLatitude()
           << std::endl
           << "           longitude: " << std::setprecision(10)
           << pos.getLongitude() << std::endl
           << "           " << pos.getHeading() << std::endl
           << "           altitude : { value: " << pos.getAltitude()
           << " || confidence: " << pos.altitude_.second << " }" << std::endl
           << "           confidenceEllipse: " << pos.confidence_ellipse_;
    return output;
}

double const
Position::getTime() const {
    return delta_time_ * TIME_CONVERSION;
}

double const
Position::getLatitude() const {
    return latitude_ * LATITUDE_UNIT_CONVERSION;
}

double const
Position::getLongitude() const {
    return longitude_ * LONGITUDE_UNIT_CONVERSION;
}

Heading const &
Position::getHeading() const {
    return theta_;
}

double const
Position::getAltitude() const {
    return altitude_.first * ALTITUDE_UNIT_CONVERSION;
}

std::string const &
Position::getAltitudeConfidence() const {
    return altitude_.second;
}

////////////////////////////////////////////////////////////////////////////////////

Acceleration::Acceleration()
    : lateral_{std::make_pair<int, int>(0, 0)},
      longitudinal_{std::make_pair<int, int>(0, 0)},
      vertical_{std::make_pair<int, int>(0, 0)}, control_{"unavailable"} {}

Acceleration::Acceleration(const rapidjson::Value &accelerations) {

    this->control_ = "unavailable";
    this->longitudinal_ = std::make_pair<int, int>(0, 0);
    this->lateral_ = std::make_pair<int, int>(0, 0);
    this->vertical_ = std::make_pair<int, int>(0, 0);

    if (accelerations.HasMember("accelerationControl"))
        this->control_ = accelerations["accelerationControl"].GetString();

    if (accelerations.HasMember("longitudinalAcceleration")) {

        const rapidjson::Value &longitudinal =
            accelerations["longitudinalAcceleration"];
        this->longitudinal_.first =
            longitudinal["longitudinalAccelerationValue"].GetInt();
        this->longitudinal_.second =
            longitudinal["longitudinalAccelerationConfidence"].GetInt();
    }

    if (accelerations.HasMember("lateralAcceleration")) {

        const rapidjson::Value &lateral = accelerations["lateralAcceleration"];
        this->lateral_.first = lateral["lateralAccelerationValue"].GetInt();
        this->lateral_.second =
            lateral["lateralAccelerationConfidence"].GetInt();
    }

    if (accelerations.HasMember("verticalAcceleration")) {

        const rapidjson::Value &vertical =
            accelerations["verticalAcceleration"];

        this->vertical_.first = vertical["verticalAccelerationValue"].GetInt();
        this->vertical_.second =
            vertical["verticalAccelerationConfidence"].GetInt();
    }
}

Acceleration::Acceleration(std::string &&control,
                           std::pair<int, int> &&longitudinal,
                           std::pair<int, int> &&lateral,
                           std::pair<int, int> &&vertical) {

    this->control_ = control;
    this->longitudinal_ = longitudinal;
    this->lateral_ = lateral;
    this->vertical_ = vertical;
}

// Copy constructor
Acceleration::Acceleration(const Acceleration &other)
    : control_(other.control_), lateral_(other.lateral_),
      longitudinal_(other.longitudinal_), vertical_(other.vertical_) {}

// Move constructor
Acceleration::Acceleration(Acceleration &&other)
    : control_(std::move(other.control_)), lateral_(std::move(other.lateral_)),
      longitudinal_(std::move(other.longitudinal_)),
      vertical_(std::move(other.vertical_)) {}

// Copy assignment operator
Acceleration &
Acceleration::operator=(const Acceleration &other) {

    if (this != &other) {
        control_ = other.control_;
        lateral_ = other.lateral_;
        longitudinal_ = other.longitudinal_;
        vertical_ = other.vertical_;
    }
    return *this;
}

// Move assignment operator
Acceleration &
Acceleration::operator=(Acceleration &&other) {

    if (this != &other) {
        control_ = std::move(other.control_);
        longitudinal_ = std::move(other.longitudinal_);
        lateral_ = std::move(other.lateral_);
        vertical_ = std::move(other.vertical_);
    }
    return *this;
}

std::ostream &
operator<<(std::ostream &output, const Acceleration &ac) {

    output << "control: " << ac.control_
           << " || longitudinal: { value: " << ac.getLongitudinal()
           << " || confidence: " << ac.longitudinal_.second << " }"
           << " || lateral: { value: " << ac.getLateral()
           << " || confidence: " << ac.lateral_.second << " }"
           << " || vertical: { value: " << ac.getVertical()
           << " || confidence: " << ac.vertical_.second << " }";
    return output;
}

std::string const &
Acceleration::getControl() const {
    return control_;
}

double const
Acceleration::getLongitudinal() const {
    return longitudinal_.first * ACCELERATION_UNIT_CONVERSION;
}

int const
Acceleration::getLongitudinalConfidence() const {
    return longitudinal_.second;
}

double const
Acceleration::getLateral() const {
    return lateral_.first * ACCELERATION_UNIT_CONVERSION;
}

int const
Acceleration::getLateralConfidence() const {
    return lateral_.second;
}

double const
Acceleration::getVertical() const {
    return vertical_.first * ACCELERATION_UNIT_CONVERSION;
}

int const
Acceleration::getVerticalConfidence() const {
    return vertical_.second;
}

////////////////////////////////////////////////////////////////////////////////////

Heading::Heading() : heading_{std::make_pair<int, int>(0, 0)} {}

Heading::Heading(const rapidjson::Value &message) {

    this->heading_ = std::make_pair<int, int>(0, 0);

    if (message.HasMember("heading")) {

        const rapidjson::Value &heading = message["heading"];

        this->heading_.first = heading["headingValue"].GetInt();
        this->heading_.second = heading["headingConfidence"].GetInt();
    }
}

Heading::Heading(int value, int confidence)
    : heading_{
          std::make_pair<int, int>(std::move(value), std::move(confidence))} {}

// Copy constructor
Heading::Heading(const Heading &other) : heading_(other.heading_) {}

// Move constructor
Heading::Heading(Heading &&other) : heading_(std::move(other.heading_)) {}

// Copy assignment operator
Heading &
Heading::operator=(const Heading &other) {

    if (this != &other) {
        heading_ = other.heading_;
    }

    return *this;
}

// Move assignment operator
Heading &
Heading::operator=(Heading &&other) {

    if (this != &other) {
        heading_ = std::move(other.heading_);
    }
    return *this;
}

std::ostream &
operator<<(std::ostream &output, const Heading &heading) {

    output << "heading: { value: " << heading.getValue()
           << " || confidence: " << heading.getConfidence() << " }";

    return output;
}

double const
Heading::getValue() const {
    return heading_.first * HEADING_UNIT_CONVERSION;
}

int const
Heading::getConfidence() const {
    return heading_.second;
}

////////////////////////////////////////////////////////////////////////////////////

Speed::Speed() : speed_{std::make_pair<int, int>(0, 0)} {}

Speed::Speed(int value, int confidence)
    : speed_{
          std::make_pair<int, int>(std::move(value), std::move(confidence))} {}

Speed::Speed(const rapidjson::Value &message) {

    this->speed_ = std::make_pair<int, int>(0, 0);

    if (message.HasMember("speed")) {
        const rapidjson::Value &speed = message["speed"];

        this->speed_.first = speed["speedValue"].GetInt();
        this->speed_.second = speed["speedConfidence"].GetInt();
    }
}

// Copy constructor
Speed::Speed(const Speed &other) : speed_(other.speed_) {}

// Move constructor
Speed::Speed(Speed &&other) : speed_(std::move(other.speed_)) {}

// Copy assignment operator
Speed &
Speed::operator=(const Speed &other) {

    if (this != &other) {
        speed_ = other.speed_;
    }
    return *this;
}

// Move assignment operator definition
Speed &
Speed::operator=(Speed &&other) {

    if (this != &other) {
        speed_ = std::move(other.speed_);
    }
    return *this;
}

std::ostream &
operator<<(std::ostream &output, const Speed &speed) {

    output << "speed: { value: " << speed.getValue()
           << " || confidence: " << speed.getConfidence() << " }";

    return output;
}

double const
Speed::getValue() const {
    return speed_.first * SPEED_UNIT_CONVERSION;
}

int const
Speed::getConfidence() const {
    return speed_.second;
}

////////////////////////////////////////////////////////////////////////////////////

Curvature::Curvature() : curvature_{std::make_pair<int, std::string>(0, "")} {}

Curvature::Curvature(int value, std::string &&confidence)
    : curvature_{std::make_pair<int, std::string>(std::move(value),
                                                  std::move(confidence))} {}

Curvature::Curvature(const rapidjson::Value &message) {

    this->curvature_ = std::make_pair<int, std::string>(0, "unavailable");

    if (message.HasMember("curvature")) {

        const rapidjson::Value &curvature = message["curvature"];

        this->curvature_.first = curvature["curvatureValue"].GetInt();
        this->curvature_.second = curvature["curvatureConfidence"].GetString();
    }
}

// Copy constructor
Curvature::Curvature(const Curvature &other) : curvature_(other.curvature_) {}

// Move constructor
Curvature::Curvature(Curvature &&other)
    : curvature_(std::move(other.curvature_)) {}

// Copy assignment operator
Curvature &
Curvature::operator=(const Curvature &other) {

    if (this != &other) {
        curvature_ = other.curvature_;
    }
    return *this;
}

// Move assignment operator
Curvature &
Curvature::operator=(Curvature &&other) {

    if (this != &other) {
        curvature_ = std::move(other.curvature_);
    }
    return *this;
}

std::ostream &
operator<<(std::ostream &output, const Curvature &curvature) {

    output << "curvature: { value: " << curvature.getValue()
           << " || confidence: " << curvature.getConfidence() << " }";

    return output;
}

int const
Curvature::getValue() const {
    return curvature_.first;
}

std::string const &
Curvature::getConfidence() const {
    return curvature_.second;
}

////////////////////////////////////////////////////////////////////////////////////

YawRate::YawRate() : yaw_rate_{std::make_pair<int, std::string>(0, "")} {}

YawRate::YawRate(int value, std::string &&confidence)
    : yaw_rate_{std::make_pair<int, std::string>(std::move(value),
                                                 std::move(confidence))} {}

YawRate::YawRate(const rapidjson::Value &message) {

    this->yaw_rate_ = std::make_pair<int, std::string>(0, "unavailable");

    if (message.HasMember("yawRate")) {
        const rapidjson::Value &yaw_rate = message["yawRate"];

        this->yaw_rate_.first = yaw_rate["yawRateValue"].GetInt();
        this->yaw_rate_.second = yaw_rate["yawRateConfidence"].GetString();
    }
}

// Copy constructor
YawRate::YawRate(const YawRate &other) : yaw_rate_(other.yaw_rate_) {}

// Move constructor
YawRate::YawRate(YawRate &&other) : yaw_rate_(std::move(other.yaw_rate_)) {}

// Copy assignment operator
YawRate &
YawRate::operator=(const YawRate &other) {

    if (this != &other) {
        yaw_rate_ = other.yaw_rate_;
    }
    return *this;
}

// Move assignment operator definition
YawRate &
YawRate::operator=(YawRate &&other) {

    if (this != &other) {
        yaw_rate_ = std::move(other.yaw_rate_);
    }
    return *this;
}

std::ostream &
operator<<(std::ostream &output, const YawRate &yaw_rate) {

    output << "yaw_rate: { value: " << yaw_rate.getValue()
           << " || confidence: " << yaw_rate.getConfidence() << " }";

    return output;
}

double const
YawRate::getValue() const {
    return yaw_rate_.first * YAW_RATE_UNIT_CONVERSION;
}

std::string const &
YawRate::getConfidence() const {
    return yaw_rate_.second;
}

////////////////////////////////////////////////////////////////////////////////////

SteeringWheelAngle::SteeringWheelAngle()
    : steering_w_a_{std::make_pair<int, int>(0, 0)} {}

SteeringWheelAngle::SteeringWheelAngle(int value, int confidence)
    : steering_w_a_{
          std::make_pair<int, int>(std::move(value), std::move(confidence))} {}

SteeringWheelAngle::SteeringWheelAngle(const rapidjson::Value &message) {

    this->steering_w_a_ = std::make_pair<int, int>(0, 0);

    if (message.HasMember("steeringWheelAngle")) {
        const rapidjson::Value &steering_w_a = message["steeringWheelAngle"];

        this->steering_w_a_.first =
            steering_w_a["steeringWheelAngleValue"].GetInt();
        this->steering_w_a_.second =
            steering_w_a["steeringWheelAngleConfidence"].GetInt();
    }
}

// Copy constructor
SteeringWheelAngle::SteeringWheelAngle(const SteeringWheelAngle &other)
    : steering_w_a_(other.steering_w_a_) {}

// Move constructor
SteeringWheelAngle::SteeringWheelAngle(SteeringWheelAngle &&other)
    : steering_w_a_(std::move(other.steering_w_a_)) {}

// Copy assignment operator
SteeringWheelAngle &
SteeringWheelAngle::operator=(const SteeringWheelAngle &other) {
    if (this != &other) {
        steering_w_a_ = other.steering_w_a_;
    }
    return *this;
}

// Move assignment operator
SteeringWheelAngle &
SteeringWheelAngle::operator=(SteeringWheelAngle &&other) {
    if (this != &other) {
        steering_w_a_ = std::move(other.steering_w_a_);
    }
    return *this;
}

std::ostream &
operator<<(std::ostream &output, const SteeringWheelAngle &steering_w_a) {

    output << "steering_wheel_angle: { value: " << steering_w_a.getValue()
           << " || confidence: " << steering_w_a.getConfidence() << " }";

    return output;
}

double const
SteeringWheelAngle::getValue() const {
    return steering_w_a_.first * STEERING_WHEEL_ANGLE_CONVERSION;
}

int const
SteeringWheelAngle::getConfidence() const {
    return steering_w_a_.second;
}

////////////////////////////////////////////////////////////////////////////////////

}   // namespace its