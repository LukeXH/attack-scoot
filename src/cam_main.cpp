#include <iomanip>
#include <iostream>
#include <memory>
#include <thread>

#include <libcamera/libcamera.h>

using namespace libcamera;
using namespace std::chrono_literals;

// Global 
static std::shared_ptr<Camera> camera;

int main()
{
    // Code to follow
    std::unique_ptr<CameraManager> cm = std::make_unique<CameraManager>();
    cm->start();

    for (auto const &camera : cm->cameras())
        std::cout << camera->id() << std::endl;

    auto cameras = cm->cameras();
    for (auto const &camera : cameras)
        std::cout << camera->id() << std::endl;
    if (cameras.empty()) {
        std::cout << "No cameras were identified on the system."
                << std::endl;
        cm->stop();
        return EXIT_FAILURE;
    }

    
    std::string cameraId = cameras[0]->id();

    auto camera = cm->get(cameraId);
    // camera = cm->get('/base/soc/i2c0mux/i2c@1/ov5647@36');
    camera->acquire();

    // Configure the camera
    std::unique_ptr<CameraConfiguration> config = camera->generateConfiguration( { StreamRole::Viewfinder } );

    // StreamConfiguration &streamConfig = config->at(0);
    StreamConfiguration &streamConfig = config->at(0);
    std::cout << "Default viewfinder configuration is: " << streamConfig.toString() << std::endl;

    return 0;
}