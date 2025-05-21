#ifndef ITS_UTILS_HPP_
#define ITS_UTILS_HPP_

#include "rapidjson/document.h"
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace its {

////////////////////////////////////////////////////////////////////////////////////

static const double MEAN_DELTA_TIME = 0.2;

////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Support function. Get the Document object from a given path.
 *
 * @param path
 * @return rapidjson::Document
 */
rapidjson::Document getDocument(const std::string &path);

////////////////////////////////////////////////////////////////////////////////////

enum class StationClassificationType {

    unknown,
    pedestrian,
    cyclist,
    moped,
    motorcycle,
    passengerCar,
    bus,
    lightTruck,
    heavyTruck,
    trailer,
    specialVehicles,
    tram,
    roadSideUnit

};

////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Heading class represents the heading of a vehicle.
 */
class Heading {

  public:
    /**
     * @brief Default constructor of Heading object
     */
    Heading();

    /**
     * @brief Construct a new Heading object from a rapidjson message.
     * @param high_container Entry corresponding to the high container. This
     */
    Heading(const rapidjson::Value &high_container);

    /**
     * @brief Construct a new Heading object from int values.
     * @param value The heading value.
     * @param confidence The confidence level of the heading.
     */
    Heading(int value, int confidence);

    /**
     * @brief Copy constructor
     * @param other The Heading object to be copied.
     */
    Heading(const Heading &other);

    /**
     * @brief Move constructor
     * @param other The Heading object to be moved.
     */
    Heading(Heading &&other);

    /**
     * @brief Copy assignment operator
     * @param other The Heading object to be copied.
     * @return Heading& The reference to the assigned Heading object.
     */
    Heading &operator=(const Heading &other);

    /**
     * @brief Move assignment operator
     * @param other The Heading object to be moved.
     * @return Heading& The reference to the assigned Heading object.
     */
    Heading &operator=(Heading &&other);

    /**
     * @brief Destroy the Heading object
     */
    ~Heading() {}

    /**
     * @brief << Operator overloading to stamp
     * @param output The output stream.
     * @param heading The Heading object to be printed.
     * @return std::ostream& The reference to the output stream.
     */
    friend std::ostream &operator<<(std::ostream &output,
                                    const Heading &heading);

    /**
     * @brief Get the Value object converted using the corresponding unit.
     * @return double const
     */
    double const getValue() const;

    /**
     * @brief Get the Confidence object converted using the corresponding unit.
     * @return int const
     */
    int const getConfidence() const;

  private:
    std::pair<int, int> heading_;   // <value, confidence>
};

////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief PositionConfidence struct
 */
struct PositionConfidence {

    /**
     * @brief Default constructor of PositionConfidence object.
     */
    PositionConfidence();

    /**
     * @brief Construct a new PositionConfidence object from int values.
     * @param semi_major The confidence level of the semi-major axis.
     * @param semi_minor The confidence level of the semi-minor axis.
     * @param orientation The confidence level of the orientation.
     */
    PositionConfidence(int semi_major, int semi_minor, int orientation);

    /**
     * @brief << operator overloading to to print the PositionConfidence object.
     * @param output The output stream.
     * @param pos The PositionConfidence object to be printed.
     * @return std::ostream& The reference to the output stream.
     */
    friend std::ostream &operator<<(std::ostream &output,
                                    const PositionConfidence &pos);

    int semi_major_confidence_;
    int semi_minor_confidence_;
    int semi_minor_orientation_;
};

/**
 * @brief Position class represents the position of a vehicle.
 */
class Position {

  public:
    /**
     * @brief Default constructor of Position object.
     */
    Position();

    /**
     * @brief Construct a new Position object from a rapidjson message, delta
     * time and heading.
     * @param basic_container Entry corresponding to the basic
     * container.
     * @param delta_time Time of CAM message creation.
     * @param heading heading of the vehicle.
     */
    Position(const rapidjson::Value &basic_container, unsigned int delta_time,
             const Heading &heading);

    /**
     * @brief Construct a new Position object from values.
     * @param delta_time Time of CAM message creation.
     * @param latitude The latitude value.
     * @param longitude The longitude value.
     * @param heading The heading value.
     * @param altitude The altitude value.
     * @param confidence_ellipse The confidence ellipse of the position.
     */
    Position(unsigned int delta_time, int latitude, int longitude,
             const std::pair<int, int> &heading,
             std::pair<int, std::string> &&altitude,
             PositionConfidence &&confidence_ellipse);

    /**
     * @brief Construct a new Position object from not-converted value of
     * latitude and longitude. The conversion is made and int-values is stored.
     * @param latitude The latitude value.
     * @param longitude The longitude value.
     */
    Position(double latitude, double longitude);

    /**
     * @brief Destroy the Position object
     */
    ~Position(){};

    /**
     * @brief Copy constructor.
     * @param other The Position object to be copied.
     */
    Position(const Position &other);

    /**
     * @brief  Copy assignment operator.
     * @param other The Position object to be copied.
     * @return Position& The reference to the assigned Position object.
     */
    Position &operator=(const Position &other);

    /**
     * @brief Move constructor.
     * @param other The Position object to be moved.
     */
    Position(Position &&other);

    /**
     * @brief Move assignment operator.
     * @param other The Position object to be moved.
     * @return Position& The reference to the assigned Position object.
     */
    Position &operator=(Position &&other);

    /**
     * @brief << operator overloading to stamp.
     * @param output The output stream.
     * @param pos The Position object to be printed.
     * @return std::ostream& The reference to the output stream.
     */
    friend std::ostream &operator<<(std::ostream &output, const Position &pos);

    /**
     * @brief Get the Time.
     * @return double const Time of the position.
     */
    double const getTime() const;

    /**
     * @brief Get the Latitude.
     * @return double const Latitude value.
     */
    double const getLatitude() const;

    /**
     * @brief Get the Longitude.
     * @return double const Longitude value.
     */
    double const getLongitude() const;

    /**
     * @brief Get the Heading.
     * @return Heading const& Heading object.
     */
    Heading const &getHeading() const;

    /**
     * @brief Get the Altitude.
     * @return double const Altitude value.
     */
    double const getAltitude() const;

    /**
     * @brief Get the Altitude Confidence.
     * @return std::string const& Confidence value.
     */
    std::string const &getAltitudeConfidence() const;

    /**
     * @brief Calculate distance metric between this position and a target one.
     * @param other Position target.
     * @return double Distance metric.
     */
    double calculateDistance(const Position &other);

    /**
     * @brief From the current values of longitude and latitude create an array
     * with those values.
     * @return std::array<double, 2> Array with longitude and latitude position.
     */
    std::array<double, 2> const getArrayPosition() const;

    /**
     * @brief Update the position with the most updated values.
     * @param latitude The latitude value.
     * @param longitude The longitude value.
     * @param heading The heading value.
     */
    void step(double latitude, double longitude, double heading);

  private:
    unsigned int delta_time_;
    int latitude_;
    int longitude_;
    std::pair<int, std::string> altitude_;   // <value, confidence>
    Heading theta_;
    PositionConfidence confidence_ellipse_;
};

////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Acceleration class represents the acceleration of a vehicle.
 */
class Acceleration {

  public:
    /**
     * @brief Construct a new Acceleration
     */
    Acceleration();

    /**
     * @brief Construct a new Acceleration from rapidjson message.
     * @param high_container Entry corresponding to the high container
     */
    Acceleration(const rapidjson::Value &high_container);

    /**
     * @brief Construct a new Acceleration from values
     * @param control The control of the acceleration.
     * @param longitudinal The longitudinal acceleration.
     * @param lateral The lateral acceleration.
     * @param vertical The vertical acceleration.
     */
    Acceleration(std::string &&control, std::pair<int, int> &&longitudinal,
                 std::pair<int, int> &&lateral, std::pair<int, int> &&vertical);

    /**
     * @brief Copy constructor
     * @param other The Acceleration object to be copied.
     */
    Acceleration(const Acceleration &other);

    /**
     * @brief Move constructor
     * @param other The Acceleration object to be moved.
     */
    Acceleration(Acceleration &&other);

    /**
     * @brief Copy assignment operator
     * @param other The Acceleration object to be copied.
     * @return Acceleration& The reference to the assigned Acceleration object.
     */
    Acceleration &operator=(const Acceleration &other);

    /**
     * @brief Move assignment operator
     * @param other The Acceleration object to be moved.
     * @return Acceleration& The reference to the assigned Acceleration object.
     */
    Acceleration &operator=(Acceleration &&other);

    /**
     * @brief << Operator overloading to print the Acceleration object.
     * @param output The output stream.
     * @param ac The Acceleration object to be printed.
     * @return std::ostream& The reference to the output stream.
     */
    friend std::ostream &operator<<(std::ostream &output,
                                    const Acceleration &ac);

    /**
     * @brief Get the control of the acceleration.
     * @return std::string const& The reference to the control value.
     */
    std::string const &getControl() const;

    /**
     * @brief Get the longitudinal acceleration value.
     * @return double const The longitudinal acceleration value.
     */
    double const getLongitudinal() const;

    /**
     * @brief Get the longitudinal acceleration confidence level.
     * @return int const The longitudinal acceleration confidence level.
     */
    int const getLongitudinalConfidence() const;

    /**
     * @brief Get the lateral acceleration value.
     * @return double const The lateral acceleration value.
     */
    double const getLateral() const;

    /**
     * @brief Get the lateral acceleration confidence level.
     * @return int const The lateral acceleration confidence level.
     */
    int const getLateralConfidence() const;

    /**
     * @brief Get the vertical acceleration value.
     * @return double const The vertical acceleration value.
     */
    double const getVertical() const;

    /**
     * @brief Get the vertical acceleration confidence level.
     * @return int const The vertical acceleration confidence level.
     */
    int const getVerticalConfidence() const;

  private:
    std::string control_;
    std::pair<int, int> longitudinal_;   // <value, confidence>
    std::pair<int, int> lateral_;        // <value, confidence>
    std::pair<int, int> vertical_;       // <value, confidence>
};

////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Speed class represents the speed of a vehicle.
 */
class Speed {

  public:
    /**
     * @brief Default constructor of Speed object.
     */
    Speed();

    /**
     * @brief Construct a new Speed object from a rapidjson message.
     * @param speed Entry corresponding to the high container
     */
    Speed(const rapidjson::Value &high_container);

    /**
     * @brief Construct a new Speed object from values.
     * @param value The value of the speed.
     * @param confidence The confidence level of the speed.
     */
    Speed(int value, int confidence);

    /**
     * @brief Copy constructor.
     * @param other The Speed object to be copied.
     */
    Speed(const Speed &other);

    /**
     * @brief Move constructor.
     * @param other The Speed object to be moved.
     */
    Speed(Speed &&other);

    /**
     * @brief Copy assignment operator.
     * @param other The Speed object to be copied.
     * @return Speed& The reference to the assigned Speed object.
     */
    Speed &operator=(const Speed &other);

    /**
     * @brief Move assignment operator.
     * @param other The Speed object to be moved.
     * @return Speed& The reference to the assigned Speed object.
     */
    Speed &operator=(Speed &&other);

    /**
     * @brief Destroy the Speed object.
     */
    ~Speed() {}

    /**
     * @brief << Operator overloading to print the Speed object.
     * @param output The output stream.
     * @param speed The Speed object to be printed.
     * @return std::ostream& The reference to the output stream.
     */
    friend std::ostream &operator<<(std::ostream &output, const Speed &speed);

    /**
     * @brief Get the value of the speed.
     * @return double const The value of the speed.
     */
    double const getValue() const;

    /**
     * @brief Get the confidence level of the speed.
     * @return int const The confidence level of the speed.
     */
    int const getConfidence() const;

  private:
    std::pair<int, int> speed_;   // <value, confidence>
};

////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Curvature class represents the curvature of a vehicle.
 */
class Curvature {

  public:
    /**
     * @brief Construct a new Curvature object from a rapidjson message.
     * @param message Entry corresponding to the high container
     */
    Curvature(const rapidjson::Value &high_container);

    /**
     * @brief Construct a new Curvature object from values.
     * @param value The value of the curvature.
     * @param confidence The confidence level of the curvature.
     */
    Curvature(int value, std::string &&confidence);

    /**
     * @brief Default constructor of Curvature object.
     */
    Curvature();

    /**
     * @brief Copy constructor.
     * @param other The Curvature object to be copied.
     */
    Curvature(const Curvature &other);

    /**
     * @brief Move constructor.
     * @param other The Curvature object to be moved.
     */
    Curvature(Curvature &&other);

    /**
     * @brief Copy assignment operator.
     * @param other The Curvature object to be copied.
     * @return Curvature& The reference to the assigned Curvature object.
     */
    Curvature &operator=(const Curvature &other);

    /**
     * @brief Move assignment operator.
     * @param other The Curvature object to be moved.
     * @return Curvature& The reference to the assigned Curvature object.
     */
    Curvature &operator=(Curvature &&other);

    /**
     * @brief Destroy the Curvature object.
     */
    ~Curvature() {}

    /**
     * @brief << Operator overloading to print the Curvature object.
     * @param output The output stream.
     * @param curvature The Curvature object to be printed.
     * @return std::ostream& The reference to the output stream.
     */
    friend std::ostream &operator<<(std::ostream &output,
                                    const Curvature &curvature);

    /**
     * @brief Get the value of the curvature.
     * @return int const The value of the curvature.
     */
    int const getValue() const;

    /**
     * @brief Get the confidence level of the curvature.
     * @return std::string const& The reference to the confidence level of the
     * curvature.
     */
    std::string const &getConfidence() const;

  private:
    std::pair<int, std::string> curvature_;   // <value, confidence>
};

////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief YawRate class represents the yaw rate of a vehicle.
 */
class YawRate {

  public:
    /**
     * @brief Construct a new YawRate object from a rapidjson message.
     * @param message Entry corresponding to the high container
     */
    YawRate(const rapidjson::Value &high_container);

    /**
     * @brief Construct a new YawRate object from values.
     * @param value The value of the yaw rate.
     * @param confidence The confidence level of the yaw rate.
     */
    YawRate(int value, std::string &&confidence);

    /**
     * @brief Default constructor of YawRate object.
     */
    YawRate();

    /**
     * @brief Copy constructor.
     * @param other The YawRate object to be copied.
     */
    YawRate(const YawRate &other);

    /**
     * @brief Move constructor.
     * @param other The YawRate object to be moved.
     */
    YawRate(YawRate &&other);

    /**
     * @brief Copy assignment operator.
     * @param other The YawRate object to be copied.
     * @return YawRate& The reference to the assigned YawRate object.
     */
    YawRate &operator=(const YawRate &other);

    /**
     * @brief Move assignment operator.
     * @param other The YawRate object to be moved.
     * @return YawRate& The reference to the assigned YawRate object.
     */
    YawRate &operator=(YawRate &&other);

    /**
     * @brief Destroy the YawRate object.
     */
    ~YawRate() {}

    /**
     * @brief << Operator overloading to print the YawRate object.
     * @param output The output stream.
     * @param yaw_rate The YawRate object to be printed.
     * @return std::ostream& The reference to the output stream.
     */
    friend std::ostream &operator<<(std::ostream &output,
                                    const YawRate &yaw_rate);

    /**
     * @brief Get the value of the yaw rate.
     * @return double const The value of the yaw rate.
     */
    double const getValue() const;

    /**
     * @brief Get the confidence level of the yaw rate.
     * @return std::string const& The reference to the confidence level of the
     * yaw rate.
     */
    std::string const &getConfidence() const;

  private:
    std::pair<int, std::string> yaw_rate_;
};

////////////////////////////////////////////////////////////////////////////////////

class SteeringWheelAngle {

  public:
    /**
     * @brief Construct a new SteeringWheelAngle object from a rapidjson
     * message.
     * @param message Entry corresponding to the high container.
     */
    SteeringWheelAngle(const rapidjson::Value &high_container);

    /**
     * @brief Construct a new SteeringWheelAngle object from values.
     * @param value The value of the steering wheel angle.
     * @param confidence The confidence level of the steering wheel angle.
     */
    SteeringWheelAngle(int value, int confidence);

    /**
     * @brief Default constructor of SteeringWheelAngle object.
     */
    SteeringWheelAngle();

    /**
     * @brief Copy constructor.
     * @param other The SteeringWheelAngle object to be copied.
     */
    SteeringWheelAngle(const SteeringWheelAngle &other);

    /**
     * @brief Move constructor.
     * @param other The SteeringWheelAngle object to be moved.
     */
    SteeringWheelAngle(SteeringWheelAngle &&other);

    /**
     * @brief Copy assignment operator.
     * @param other The SteeringWheelAngle object to be copied.
     * @return SteeringWheelAngle& The reference to the assigned
     * SteeringWheelAngle object.
     */
    SteeringWheelAngle &operator=(const SteeringWheelAngle &other);

    /**
     * @brief Move assignment operator.
     * @param other The SteeringWheelAngle object to be moved.
     * @return SteeringWheelAngle& The reference to the assigned
     * SteeringWheelAngle object.
     */
    SteeringWheelAngle &operator=(SteeringWheelAngle &&other);

    /**
     * @brief Destroy the SteeringWheelAngle object.
     */
    ~SteeringWheelAngle() {}

    /**
     * @brief << Operator overloading to print the SteeringWheelAngle object.
     * @param output The output stream.
     * @param steering_w_a The SteeringWheelAngle object to be printed.
     * @return std::ostream& The reference to the output stream.
     */
    friend std::ostream &operator<<(std::ostream &output,
                                    const SteeringWheelAngle &steering_w_a);

    /**
     * @brief Get the value of the steering wheel angle.
     * @return double const The value of the steering wheel angle.
     */
    double const getValue() const;

    /**
     * @brief Get the confidence level of the steering wheel angle.
     * @return int const The confidence level of the steering wheel angle.
     */
    int const getConfidence() const;

  private:
    std::pair<int, int> steering_w_a_;   // <value, confidence>
};

////////////////////////////////////////////////////////////////////////////////////

}   // namespace its

#endif   // ITS_UTILS_HPP_