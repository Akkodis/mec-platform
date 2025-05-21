#include "../../include/its.hpp"
#include "../../include/its_utils.hpp"
#include "../../include/local_ms_detector.hpp"

#include "MLW_AMQP.hpp"
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include <boost/circular_buffer.hpp>
#include <gtest/gtest.h>

std::unique_ptr<LocalMSDetector> detector = std::make_unique<LocalMSDetector>();
unsigned int station_id;

void
on_CAM_received(proton::delivery d, proton::message msg) {

    const std::string msg_payload = proton::get<std::string>(msg.body());
    rapidjson::Document cam_message;

    cam_message.Parse(msg_payload.c_str());

    station_id = cam_message["header"]["stationID"].GetInt();

    if (!detector->contains(station_id)) {
        // new station arrives, detectors are not yet activated
        detector->addNewStation(station_id, 0, cam_message);
        std::cout << "New station updated." << std::endl;
        return;
    }

    detector->updateStation(station_id, cam_message);
    std::cout << "Update." << std::endl;
}

struct addMultipleITSTest {

    AMQP_1_0::Subscriber sub_;
    proton::container container_1_;
    const std::string AMQP_ADDRESS = "130.192.86.38:5672";
    const std::string AMQP_USERNAME = "links";
    const std::string AMQP_PASSWORD = "cs1c74K";
    const std::string AMQP_TOPIC = "cam_simulator";

    addMultipleITSTest() {
        this->sub_.set_subscriber(AMQP_ADDRESS, AMQP_USERNAME, AMQP_PASSWORD,
                                  AMQP_TOPIC);
        sub_.set_callback(on_CAM_received);
    }
};

TEST(addMultipleITSTest, add5secStation) {

    addMultipleITSTest test = addMultipleITSTest();
    proton::container container(test.sub_, "CAM Received subscriber");
    std::thread thread([&]() { container.run(); });

    std::cout << "[add5secStation]: Listening CAM messages..." << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(5));

    detector->print();

    test.sub_.close_subscriber();
    container.stop();
    thread.join();

    EXPECT_TRUE(true);
}

TEST(addMultipleITSTest, add20secStation) {

    addMultipleITSTest test = addMultipleITSTest();
    proton::container container(test.sub_, "CAM Received subscriber");
    std::thread thread([&]() { container.run(); });

    std::cout << "[add5secStation]: Listening CAM messages..." << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(15));

    detector->print();

    test.sub_.close_subscriber();
    container.stop();
    thread.join();

    EXPECT_TRUE(true);
}