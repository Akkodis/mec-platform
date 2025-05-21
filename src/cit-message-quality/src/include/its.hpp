#ifndef ITS_HPP_
#define ITS_HPP_

#include "its_utils.hpp"
#include "predictor.hpp"
#include "rapidjson/document.h"

#include <boost/circular_buffer.hpp>
#include <boost/circular_buffer/space_optimized.hpp>
#include <iostream>
#include <string>
#include <vector>

namespace its {

/**
 * @brief ITS class where informations about one vehicle are collected.
 */
class ITS {

  public:
    /**
     * @brief Default constructor.
     */
    ITS() noexcept;

    /**
     * @brief Construct a new (full) ITS from json message.
     * @param cam_message Full json document.
     */
    ITS(const rapidjson::Document &cam_message) noexcept;

    /**
     * @brief Construct a new ITS without allocate the positions circular
     * buffer.
     * @param cam_message Full json document.
     * @param empty Useless flag.
     */
    ITS(const rapidjson::Document &cam_message, bool empty) noexcept;

    /**
     * @brief Destroy the ITS object.
     */
    ~ITS(){};

    /**
     * @brief Move constructor.
     * @param other The ITS object to be moved.
     */
    ITS(ITS &&other) noexcept;

    /**
     * @brief Move assignment operator.
     * @param other The ITS object to be moved.
     * @return ITS& The reference to the assigned ITS object.
     */
    ITS &operator=(ITS &&other) noexcept;

    /**
     * @brief Copy constructor.
     * @param other The ITS object to be copied.
     */
    ITS(const ITS &other) noexcept;

    /**
     * @brief Copy assignment operator.
     * @param other The ITS object to be copied.
     * @return ITS& The reference to the assigned ITS object.
     */
    ITS &operator=(const ITS &other);

    /**
     * @brief Get the size of positions circular buffer.
     * @return int Actual size of circular buffer.
     */
    int sizePositions();

    /**
     * @brief Update the vehicle information with the most recent vehicle
     * informations
     * @param cam_message
     * @return int
     */
    int update(const its::ITS &its);

    /**
     * @brief Update the current its position with the prediction.
     */
    void updateWithPrediction();

    /**
     * @brief Predict the next position given the current.
     * @return Position Position predicted.
     */
    Position predictNextPosition();

    /**
     * @brief Get the Station Id
     * @return unsigned int const& Station Id
     */
    unsigned int const getStationId() const;

    /**
     * @brief Get the Current Position
     * @return Position const& Current position
     */
    Position const &getCurrentPosition() const;

    /**
     * @brief << operator overload.
     * @param output The output stream.
     * @param curvature The ITS object to be printed.
     * @return std::ostream& The reference to the output stream.
     */
    friend std::ostream &operator<<(std::ostream &output, const ITS &its);

  private:
    static const int MAX_POSITIONS_ = 16;

    unsigned int station_id_;
    unsigned int current_generation_delta_time_;

    StationClassificationType station_type_;
    boost::circular_buffer_space_optimized<Position> positions_;
    Position last_position_;

    std::string drive_direction_;

    Heading heading_;
    Speed speed_;
    Acceleration accelerations_;
    YawRate yaw_rate_;
    SteeringWheelAngle steering_wheel_angle_;
    Curvature curvature_;
    std::string curvature_calculation_mode_;

    std::shared_ptr<Predictor> predictor_;

    bool _debug_ = false;
};

}   // namespace its

////////////////////////////////////////////////////////////////////////////////////

#endif   // ITS_HPP_