#include "local_ms_detector.hpp"

#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/prepared_statement.h>

////////////////////////////////////////////////////////////////////////////////////

// Default constructor
LocalMSDetector::LocalMSDetector()
    : stations_{std::make_shared<std::map<unsigned int, its::ITS>>()},
      metrics_{std::make_shared<std::map<unsigned int, BatchMetrics>>()} {}

////////////////////////////////////////////////////////////////////////////////////

// Copy constructor
LocalMSDetector::LocalMSDetector(const LocalMSDetector &other)
    : LocalMSDetector() {
    stations_ = other.stations_;
    metrics_ = other.metrics_;
}

////////////////////////////////////////////////////////////////////////////////////

// Move constructor
LocalMSDetector::LocalMSDetector(LocalMSDetector &&other)
    : stations_(std::move(other.stations_)),
      metrics_(std::move(other.metrics_)) {}

////////////////////////////////////////////////////////////////////////////////////

// Move assignment
LocalMSDetector &
LocalMSDetector::operator=(LocalMSDetector &&other) {
    if (this != &other) {
        stations_ = std::move(other.stations_);
        metrics_ = std::move(other.metrics_);
    }
    return *this;
}

////////////////////////////////////////////////////////////////////////////////////

// Copy assignment definition
LocalMSDetector &
LocalMSDetector::operator=(const LocalMSDetector &other) {
    if (this != &other) {
        stations_ = other.stations_;
        metrics_ = other.metrics_;
    }

    return *this;
}

////////////////////////////////////////////////////////////////////////////////////

void
LocalMSDetector::addNewStation(unsigned int station_id,
                               unsigned int data_flow_id,
                               const rapidjson::Document &cam_message) {

    auto lock = std::lock_guard<std::mutex>(this->lock_);

    if (this->contains(station_id)) {

        std::cout
            << "LocalMSDetector::addNewStation: Station is already registred."
            << std::endl;
        return;
    }

    its::ITS new_its{cam_message};

    auto pair = std::make_pair(station_id, std::move(new_its));

    stations_->insert(std::move(pair));

    metrics_->insert(std::make_pair(station_id, BatchMetrics(data_flow_id)));
}

////////////////////////////////////////////////////////////////////////////////////

int
LocalMSDetector::updateStation(unsigned int station_id, const its::ITS &its) {

    auto lock = std::lock_guard<std::mutex>(this->lock_);

    if (!this->contains(station_id)) {

        std::cout << "LocalMSDetector::updateStation: Station is not added, "
                     "update station failed"
                  << std::endl;
        return -1;
    }

    its::ITS &station = this->stations_->find(station_id)->second;

    station.update(its);

    return 0;
}

int
LocalMSDetector::updateStationWithLastPrediction(unsigned int station_id) {

    auto lock = std::lock_guard<std::mutex>(this->lock_);

    if (!this->contains(station_id)) {

        std::cout << "LocalMSDetector::updateStation: Station is not added, "
                     "update station failed"
                  << std::endl;
        return -1;
    }

    its::ITS &station = this->stations_->find(station_id)->second;

    station.updateWithPrediction();

    return 0;
}

////////////////////////////////////////////////////////////////////////////////////

int
positionConsistencyMetric(double metric) {

    if (metric < 0.5)
        return 7;
    else if (metric >= 0.5 && metric < 1)
        return 6;
    else if (metric >= 1 && metric < 2)
        return 5;
    else if (metric >= 2 && metric < 5)
        return 4;
    else if (metric >= 5 && metric < 10)
        return 3;
    else if (metric >= 10 && metric < 20)
        return 2;
    else
        return 1;
}

////////////////////////////////////////////////////////////////////////////////////

int
LocalMSDetector::detectPositionConsistency(const its::ITS &new_station) {

    unsigned int station_id = new_station.getStationId();

    // Check if the station is registered
    if (!this->contains(station_id)) {

        std::cout << "LocalMSDetector::detectPositionConsistency: Station is "
                     "not registered."
                  << std::endl;
        return 0;
    }

    // Next position prediction
    its::ITS &station =
        this->stations_->find(station_id)->second;   // TO DO: check this copy.

    // The detector start working with at least two positions.
    if (station.sizePositions() < 2)
        return 7;

    its::Position prediction = station.predictNextPosition();

    if (true) {
        std::cout << "///////////////////////////////////" << std::endl;

        std::cout << "Actual position:" << std::endl
                  << station.getCurrentPosition() << std::endl;
        std::cout << "Predicted position:" << std::endl
                  << prediction << std::endl;
        std::cout << "New position:" << std::endl
                  << new_station.getCurrentPosition() << std::endl;

        std::cout << "Distance: "
                  << prediction.calculateDistance(
                         new_station.getCurrentPosition())
                  << std::endl;

        std::cout << "///////////////////////////////////" << std::endl;
    }

    // Calculate distance metric from predition and
    double distance =
        prediction.calculateDistance(new_station.getCurrentPosition());

    return convertMetrics(DetectorsInfo::metrics::POSITION_CONSISTENCY,
                          distance);
}

////////////////////////////////////////////////////////////////////////////////////

int
LocalMSDetector::convertMetrics(DetectorsInfo::metrics type, double metric) {

    switch (type) {
    case DetectorsInfo::metrics::POSITION_CONSISTENCY:
        return positionConsistencyMetric(metric);

        // insert other metrics ...
    };

    return 7;
}

////////////////////////////////////////////////////////////////////////////////////

int
weightedMean(const std::vector<int> &r) {

    double ret = 0;

    for (int i = 0; i < DetectorsInfo::N_DETECTORS; i++)
        ret += DetectorsInfo::weights[i] * r[i];

    ret = ret / (std::accumulate(DetectorsInfo::weights.begin(),
                                 DetectorsInfo::weights.end(), 0));

    return static_cast<int>(ret);
}

////////////////////////////////////////////////////////////////////////////////////

int
LocalMSDetector::detect(const its::ITS &new_station) {

    auto lock = std::lock_guard<std::mutex>(this->lock_);

    std::vector<int> result_detectors{DetectorsInfo::N_DETECTORS};
    std::vector<std::future<int>> vector_threads;

    // declare routines to be executed by threads asynchronously
    auto f_detect_position_consistency =
        std::bind(&LocalMSDetector::detectPositionConsistency, this,
                  std::placeholders::_1);

    // insert other detector's routine
    // ...

    // launch threads
    vector_threads.push_back(std::async(std::launch::async,
                                        f_detect_position_consistency,
                                        std::cref(new_station)));
    // insert other detectors
    // ...

    for (int i = 0; i < DetectorsInfo::N_DETECTORS; i++)
        result_detectors[i] = vector_threads[i].get();

    return weightedMean(result_detectors);
}

////////////////////////////////////////////////////////////////////////////////////

bool
LocalMSDetector::contains(unsigned int station_id) {

    return this->stations_->count(station_id) != 0;
}

////////////////////////////////////////////////////////////////////////////////////

void
LocalMSDetector::print(unsigned int station_id) {

    auto lock = std::lock_guard<std::mutex>(this->lock_);

    if (station_id == -1) {

        if (this->stations_->empty())
            return;

        std::cout << "LocalMSDetector::print: Printing all database..."
                  << std::endl;

        for (auto it = this->stations_->begin(); it != this->stations_->end();
             ++it) {

            auto &its = it->second;
            std::cout << its << std::endl;
        }

    } else {

        if (!this->contains(station_id))
            return;

        std::cout << "LocalMSDetector::print: Printing all database "
                     "corresponding to station Id ["
                  << station_id << "] ..." << std::endl;

        auto &its = this->stations_->find(station_id)->second;

        std::cout << its << std::endl;
    }
}

////////////////////////////////////////////////////////////////////////////////////

void
LocalMSDetector::updateAndVisualizeBatchMetrics(unsigned int station_id,
                                                int metric) {

    auto lock = std::lock_guard<std::mutex>(this->lock_);

    if (this->metrics_->count(station_id) == 0) {
        this->metrics_->insert(std::make_pair(station_id, BatchMetrics()));
    }

    auto &metric_id = this->metrics_->find(station_id)->second;

    metric_id.update(metric);
    metric_id.publish(station_id);
}

////////////////////////////////////////////////////////////////////////////////////

BatchMetrics::BatchMetrics() : iteration_{0}, metric_{0}, data_flow_id{0} {}

BatchMetrics::BatchMetrics(unsigned int data_flow_id)
    : iteration_{0}, metric_{0}, data_flow_id{data_flow_id} {}

void
BatchMetrics::reset() {

    iteration_ = 0;
    metric_ = 0;
}

void
BatchMetrics::update(int metric) {

    this->metric_ += metric;
    this->iteration_++;
}

void
BatchMetrics::publish(unsigned int station_id) {

    if (iteration_ >= DetectorsInfo::BATCH_SIZE) {

        std::string DB_ADDRESS;
        std::string DB_USERNAME;
        std::string DB_PASSWORD;

        try {
            DB_ADDRESS = getenv("DB_ADDRESS");
            DB_USERNAME = getenv("DB_USERNAME");
            DB_PASSWORD = getenv("DB_PASSWORD");
        } catch (...) {
            std::cout << "Error on env variables." << std::endl;
        }
        
        if ( DB_ADDRESS.empty() || DB_USERNAME.empty() || DB_PASSWORD.empty() ){
            std::cout << "NULL env variables." << std::endl;
            exit(1);
        }

        // calculate the result
        int res = static_cast<int>(metric_ / iteration_);

        // print the result
        std::cout << "[ID]: [" << station_id << "] | "
                  << " [DATAFLOWID]: [" << data_flow_id
                  << "]: Detectors estimation mean value: " << res << std::endl;

        // reset

        sql::mysql::MySQL_Driver *driver;
        sql::Connection *con;

        // Create a MySQL Connector/C++ driver
        driver = sql::mysql::get_mysql_driver_instance();

        // Establish a connection to the MySQL database
        con = driver->connect(DB_ADDRESS, DB_USERNAME, DB_PASSWORD);

        // Select the database
        con->setSchema("dataflowdb");

        // Prepare the SQL statement to update the quality
        sql::PreparedStatement *stmt;
        stmt = con->prepareStatement("UPDATE dataflows SET quality = ? WHERE dataflowId = ?");

        // Set the quality and dataflowId parameters
        stmt->setInt(1, res);
        stmt->setInt(2, data_flow_id);

        // Execute the update statement
        stmt->executeUpdate();

        // Clean up resources
        delete stmt;
        delete con;


        this->reset();
    }
}