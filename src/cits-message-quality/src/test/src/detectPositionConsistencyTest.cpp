#include "../../include/its.hpp"
#include "../../include/its_utils.hpp"
#include "../../include/predictor.hpp"
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include <boost/circular_buffer.hpp>
#include <gtest/gtest.h>
#include <gtsam/slam/dataset.h>

its::Position
readFile(const std::string &path) {

    rapidjson::Document d = its::getDocument(path);

    const rapidjson::Value &cam_basic_container =
        d["cam"]["camParameters"]["basicContainer"];
    const rapidjson::Value &cam_high_container =
        d["cam"]["camParameters"]["highFrequencyContainer"]
         ["basicVehicleContainerHighFrequency"];

    its::Heading heading_ = its::Heading(cam_high_container["heading"]);

    its::Position pos =
        its::Position{cam_basic_container,
                      d["cam"]["generationDeltaTime"].GetUint(), heading_};

    return pos;
}

struct detectPositionConsistencyTest : public ::testing::Test {

    static const int MAX_POSITIONS = 8;

    boost::circular_buffer_space_optimized<its::Position> positions{
        MAX_POSITIONS};


    its::ITS setup(const int N_POSITION) {
        its::ITS its;

        for (int i = 0; i < N_POSITION; i++) {
            std::string path = "/workspaces/its_detectors/resource/";
            path.append(std::to_string(i));
            path.append(".json");

            rapidjson::Document d = its::getDocument(path);

            if (i == 0)
                its = its::ITS{d};
            else
                its.update(d);
        }

        return its;
    }
};

TEST_F(detectPositionConsistencyTest, predict1Position) {

    std::cout << "[TEST]: Predict1Position" << std::endl;

    std::string path = "/workspaces/its_detectors/resource/2.json";
    its::ITS its = setup(1);

    its::Position actualPos = readFile(path);

    std::cout << "Actual position:" << std::endl;
    std::cout << actualPos << std::endl;

    its::Position predictionPos = its.predictNextPosition();
    std::cout << "Predicted position: " << std::endl;
    std::cout << predictionPos << std::endl;

    std::cout << "Distance: " << actualPos.calculateDistance(predictionPos)
              << std::endl;

    std::cout << "///////////////////////////////////////////" << std::endl;
    EXPECT_TRUE(true);
}

TEST_F(detectPositionConsistencyTest, predict3Positions) {

    std::cout << "[TEST]: Predict3Position" << std::endl;

    std::string path = "/workspaces/its_detectors/resource/3.json";
    its::ITS its = setup(2);

    its::Position actualPos = readFile(path);

    std::cout << "Actual position:" << std::endl;
    std::cout << actualPos << std::endl;

    its::Position predictionPos = its.predictNextPosition();
    std::cout << "Predicted position: " << std::endl;
    std::cout << predictionPos << std::endl;

    std::cout << "Distance: " << actualPos.calculateDistance(predictionPos)
              << std::endl;

    std::cout << "///////////////////////////////////////////" << std::endl;
    EXPECT_TRUE(true);
}

TEST_F(detectPositionConsistencyTest, predict4Positions) {

    std::cout << "[TEST]: Predict4Position" << std::endl;

    std::string path = "/workspaces/its_detectors/resource/4.json";
    its::ITS its = setup(3);

    its::Position actualPos = readFile(path);

    std::cout << "Actual position:" << std::endl;
    std::cout << actualPos << std::endl;

    its::Position predictionPos = its.predictNextPosition();
    std::cout << "Predicted position: " << std::endl;
    std::cout << predictionPos << std::endl;

    std::cout << "Distance: " << actualPos.calculateDistance(predictionPos)
              << std::endl;

    std::cout << "///////////////////////////////////////////" << std::endl;
    EXPECT_TRUE(true);
}

TEST_F(detectPositionConsistencyTest, predict6Positions) {

    std::cout << "[TEST]: Predict6Position" << std::endl;

    std::string path = "/workspaces/its_detectors/resource/6.json";
    its::ITS its = setup(5);

    std::cout << "Stamping database: " << its << std::endl;

    its::Position actualPos = readFile(path);

    std::cout << "Actual position:" << std::endl;
    std::cout << actualPos << std::endl;

    its::Position predictionPos = its.predictNextPosition();
    std::cout << "Predicted position: " << std::endl;
    std::cout << predictionPos << std::endl;

    std::cout << "Distance: " << actualPos.calculateDistance(predictionPos)
              << std::endl;

    std::cout << "///////////////////////////////////////////" << std::endl;
    EXPECT_TRUE(true);
}

// TEST_F(detectPositionConsistencyTest, configPositions) {

//     std::cout << "[TEST]: configPositions" << std::endl;

//     gtsam::NonlinearFactorGraph::shared_ptr graph;
//     gtsam::Values::shared_ptr initial;
//     gtsam::SharedDiagonal model = gtsam::noiseModel::Diagonal::Sigmas(
//         (gtsam::Vector(3) << 0.05, 0.05, 5.0 * M_PI / 180.0).finished());
//     std::string graph_file = gtsam::findExampleDataFile("w100.graph");
//     std::tie(graph, initial) = gtsam::load2D(graph_file, model);

//     // graph->print("Graph: ");

//     graph->at(1)->print("Fist: ");
//     gtsam::Pose2 first(0.995595, 0.0837204, 0.0146728);
//     gtsam::Pose2 second(2.0463, 0.0352563, -0.0332615);

//     second.between(first).print("Debug: ");

//     EXPECT_TRUE(true);
// }