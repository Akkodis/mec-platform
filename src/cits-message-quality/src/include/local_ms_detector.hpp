#ifndef LOCAL_MS_DETECTOR_HPP_
#define LOCAL_MS_DETECTOR_HPP_

#include "its.hpp"

#include "rapidjson/document.h"
#include <functional>
#include <future>
#include <iostream>
#include <map>
#include <mutex>
#include <numeric>
#include <thread>

namespace DetectorsInfo {

const static int N_DETECTORS = 1;          // Number of implemented detectors
const static int VALIDITY_THRESHOLD = 0;   // Threshold used for
const static int BATCH_SIZE = 4;          // Batch size for BatchMetrics.

enum class metrics {
    POSITION_CONSISTENCY,
};

static const std::vector<int> weights = {
    1,   // POSITION_CONSISTENCY
};

}   // namespace DetectorsInfo

/**
 * @brief BatchMetrics class where detector metrics are managed.
 */
class BatchMetrics {

  public:
    /**
     * @brief Construct a new Batch Metrics
     */
    BatchMetrics();

    /**
     * @brief Construct a new Batch Metrics with data flow id value.
     */
    BatchMetrics(unsigned int data_flow_id);

    /**
     * @brief Update with a new metric value.
     * @param metric New metric value.
     */
    void update(int metric);

    /**
     * @brief Publish the metric value.
     * @param station_id Station Id value used for print.
     */
    void publish(unsigned int station_id);

    /**
     * @brief Reset the BatchMetric.
     */
    void reset();

  private:

    unsigned int data_flow_id; 
    int metric_;
    int iteration_;
};

/**
 * @brief Local Misbeavior Message Detector class.
 */
class LocalMSDetector {

  public:
    /**
     * @brief Default constructor.
     */
    LocalMSDetector();

    /**
     * @brief Copy constructor.
     * @param other The LocalMSDetector object to be copied.
     */
    LocalMSDetector(const LocalMSDetector &other);

    /**
     * @brief Move constructor.
     * @param other The LocalMSDetector object to be moved.
     */
    LocalMSDetector(LocalMSDetector &&other);

    /**
     * @brief Copy assignment operator.
     * @param other The LocalMSDetector object to be copied.
     * @return LocalMSDetector& The reference to the assigned LocalMSDetector
     * object.
     */
    LocalMSDetector &operator=(const LocalMSDetector &other);

    /**
     * @brief Move assignment operator
     * @param other The LocalMSDetector object to be moved.
     * @return LocalMSDetector& The reference to the assigned LocalMSDetector
     * object.
     */
    LocalMSDetector &operator=(LocalMSDetector &&other);

    /**
     * @brief Destroy the LocalMSDetector object
     */
    ~LocalMSDetector() {}

    /**
     * @brief Add a new station to the database.
     * @param station_id New station_id.
     * @param data_flow_id Data flow id of the message
     * @param cam_message ITS informations.
     */
    void addNewStation(unsigned int station_id, unsigned int data_flow_id,
                       const rapidjson::Document &cam_message);

    /**
     * @brief Methods calls all the detectors asynchronously when a ITS
     * information arrives.
     * @param new_station ITS informations.
     * @return int Final metrics.
     */
    int detect(const its::ITS &new_station);

    /**
     * @brief Update station information.
     * @param station_id Station ID of the station.
     * @param its::ITS ITS informations.
     * @return int Value = {0, -1} | 0 = All working, -1 = Something goes wrong.
     */
    int updateStation(unsigned int station_id, const its::ITS &its);

    /**
     * @brief Update the its information with last predicted information.
     * @param station_id Station ID of the station.
     * @return int Value = {0, -1} | 0 = All working, -1 = Something goes wrong.
     */
    int updateStationWithLastPrediction(unsigned int station_id);

    /**
     * @brief Check if the station is already registred in the database.
     * @param station_id Station ID of the station to be checked.
     * @return true
     * @return false
     */
    bool contains(unsigned int station_id);

    /**
     * @brief Print the entire database of ITS or a precific one.
     * @param station_id Station id of the specific station to be visualized.
     */
    void print(unsigned int station_id = -1);

    /**
     * @brief Convert metrics information from passed type of Detector.
     * @param type Detector type.
     * @param metric Metric value.
     * @return int Converted metric [1,7].
     */
    int convertMetrics(DetectorsInfo::metrics type, double metric);

    /**
     * @brief Update the BatchMetrics and occasionally visualize it.
     * @param station_id Station ID of the station.
     * @param metric Metric value.
     */
    void updateAndVisualizeBatchMetrics(unsigned int station_id, int metric);

  private:
    /**
     * @brief Given a new ITS information, calculate its consistency with
     * respect to a predicted one.
     * @param new_station New ITS stations.
     * @return int Detector metric [1,7]
     */
    int detectPositionConsistency(const its::ITS &new_station);

    std::mutex lock_;

    // Map indexed with ITS station id of ITS objects.
    std::shared_ptr<std::map<unsigned int, its::ITS>> stations_;

    // Map indexed with ITS station id of BatchMetrics.
    std::shared_ptr<std::map<unsigned int, BatchMetrics>> metrics_;

    bool _debug_ = false;
};

#endif   // LOCAL_MS_DETECTOR_HPP_