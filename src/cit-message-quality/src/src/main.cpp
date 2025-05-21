#include "MLW_AMQP.hpp"
#include "its.hpp"
#include "local_ms_detector.hpp"
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include <iostream>
#include <memory>
#include <thread>
#include <csignal>

const static int SLEEP_MS = 20;

void signalHandler(int signal) {
    std::cout << "Signal " << signal << " received. Closing the program." << std::endl;
    exit(signal); // Terminate the program with the given signal
}

void on_CAMReceived(proton::delivery d, proton::message msg);
std::unique_ptr<LocalMSDetector> detector = std::make_unique<LocalMSDetector>();

int
main(int argc, char **argv) {
    
    std::signal(SIGINT, signalHandler);

    std::string AMQP_ADDRESS;
    std::string AMQP_USERNAME;
    std::string AMQP_PASSWORD;
    std::string AMQP_TOPIC;

    try {
        AMQP_ADDRESS = getenv("AMQP_ADDRESS");
        AMQP_USERNAME = getenv("AMQP_USERNAME");
        AMQP_PASSWORD = getenv("AMQP_PASSWORD");
        AMQP_TOPIC = getenv("AMQP_TOPIC");
    } catch (...) {
        std::cout << "Error on env variables." << std::endl;
    }
    
    if ( AMQP_ADDRESS.empty() || AMQP_USERNAME.empty() || AMQP_PASSWORD.empty() || AMQP_TOPIC.empty() ){
        std::cout << "NULL env variables." << std::endl;
        exit(1);
    }

    // create the AMQP subscriber.
    AMQP_1_0::Subscriber sub(AMQP_ADDRESS, AMQP_USERNAME, AMQP_PASSWORD,
                             AMQP_TOPIC);

    // set the callback
    sub.set_callback(on_CAMReceived);

    proton::container container_1(sub, "CAM Received subscriber");
    std::cout << "[MAIN]: Listening CAM messages..." << std::endl;
    std::thread main_thread([&]() { container_1.run(); });

    while (1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_MS));
    }
}

void
on_CAMReceived(proton::delivery d, proton::message msg) {

    // Get the msg payload
    const std::string &msg_payload = proton::get<std::string>(msg.body());
    rapidjson::Document cam_message;
    unsigned int station_id;
    
    // parse the cam message.
    cam_message.Parse(msg_payload.c_str());

    // extract the its ID.
    station_id = cam_message["header"]["stationID"].GetInt();

    // check if the station is already registered.
    if (!detector->contains(station_id)) {

        // get the data_flow_id of the message
        int data_flow_id = (int) proton::get<int>(msg.properties().get("dataFlowId"));

        // new station arrives, detectors are not yet activated
        detector->addNewStation(station_id, data_flow_id, cam_message);

        std::cout << "[MAIN]: CAM messages from a new station. ID: "
                  << station_id << std::endl;

        return;
    }

    // create an "empty" its, without including the vector of positions.
    its::ITS its_new{cam_message, true};

    // call all detectors.
    int res_detection = detector->detect(its_new);

    // update detectors algorithms.
    if (res_detection >= DetectorsInfo::VALIDITY_THRESHOLD)
        detector->updateStation(station_id, its_new);
    else
        detector->updateStationWithLastPrediction(station_id);

    // update and visualize the output result.
    detector->updateAndVisualizeBatchMetrics(station_id, res_detection);
}
