#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <sys/mman.h>
#include <thread>
#include <vector>

#include <libcamera/libcamera.h>

#include "include/image.hpp"

using namespace libcamera;
using namespace std::chrono_literals;

// Globals
static std::shared_ptr<Camera> camera;
static std::shared_ptr<FrameBuffer> g_fbuffer0;
static std::shared_ptr<FrameBuffer> g_fbuffer1;

static void requestComplete(Request *request)
{
    // First check if the request has completed succesfully
    // and wasn't cancelled
    if (request->status() == Request::RequestCancelled)
        return;

    const std::map<const Stream *, FrameBuffer *> &buffers = request->buffers();

    for (auto bufferPair : buffers) {
        FrameBuffer *buffer = bufferPair.second;
        const FrameMetadata &metadata = buffer->metadata();

        std::cout << " seq: " << std::setw(6) << std::setfill('0') << metadata.sequence << " bytesused: ";

        unsigned int nplane = 0;
        for (const FrameMetadata::Plane &plane : metadata.planes())
        {
            std::cout << plane.bytesused;
            if (++nplane < metadata.planes().size()) std::cout << "/";
        }

        // const FrameBuffer::Plane &im0_plane = buffer->planes()[0];
        // const FrameBuffer::Plane &im1_plane = buffer->planes()[1];

        // Now give information about the planes
        size_t im_cnt = 0;
        for (const FrameBuffer::Plane &plane: buffer->planes() )
        {
            std::cout << "\nThere is at least one plane of length: " << plane.length << std::endl;
            void *memory = mmap(NULL, plane.length, PROT_READ | PROT_WRITE, MAP_SHARED, plane.fd.get(), 0);
            libcamera::Span<uint8_t> image_raw(static_cast<uint8_t *>(memory), plane.length);
            Image8b image(480, 640*4, IM_8UC1, (uint8_t*)(image_raw.data())); // This formating works, cuz we have 4 channels, RGBA

            // // Save as CSV
            // std::ofstream im_file;
            // std::string im_file_name = "/home/scoot/logs/imlog";
            // im_file.open(im_file_name.append(std::to_string(im_cnt++)).append(".log"));
            // for (size_t i=0; i < image.rows; i++)
            // {
            //     for (size_t j=0; j < image.cols; j++)
            //     {
            //         im_file << std::to_string(image.at(i,j)) << ",";
            //     }
            //     im_file << "\n";
            // }
            // im_file.close();
            /* In theory, should be able to map to cv matrics via:*/
            // cv::Mat image(height, width, CV_8UC1, (uint8_t *)(image_raw.data()));

            //std::unique_ptr<Image> image =
            //Image::fromFrameBuffer(buffer, Image::MapMode::ReadOnly);
            //assert(image != nullptr);
            // std::cout << "Open the stream" << std::endl;
            // std::ifstream im(plane.dmabuf());
            // std::cout << "Determine the file length" << std::endl;
            // im.seekg(0, std::ios_base::end);
            // std::size_t size=im.tellg();
            // im.seekg(0, std::ios_base::beg);
            // std::cout << "Create a vector to store the data" << std::endl;
            // std::vector<float> v(size/sizeof(float));
            // std::cout << "Load the data" << std::endl;
            // im.read((char*) &v[0], size);
            // std::cout << "Close the file" << std::endl;
            // im.close();
        }

        std::cout << std::endl;
    }

    // request->reuse(Request::ReuseBuffers);
    // camera->queueRequest(request);
}

class RequestFlag
{
private:
    bool request_ready;
    size_t req_cnt;
    size_t req_cnt_tgt;
public:
    std::function<void(Request*)> cb_handler;

    // void requestComplete(Request *request); // Prototype

    RequestFlag(size_t reqs_needed) {
        this->req_cnt_tgt = reqs_needed;
        this->req_cnt = 0;
        this->request_ready = false;
        using namespace std::placeholders;
        cb_handler = std::bind(&RequestFlag::requestComplete, this, _1);
    }
    ~RequestFlag() = default;

    void requestComplete(Request *request)
    {
        // Binding function
        // First check if the request has completed succesfully
        // and wasn't cancelled
        if (request->status() == Request::RequestCancelled)
            return;

        if(!this->request_ready && (++this->req_cnt == this->req_cnt_tgt) ) // Only flip if we are waiting for a request to be ready
            this->request_ready = true;
    }

    bool ready() const
    {
        return this->request_ready;
    }

    void reset()
    {
        this->request_ready = false;
        this->req_cnt = 0;
    }
};


/* MAIN */
int main()
{
    Image8b im;

    // Code to follow
    std::unique_ptr<CameraManager> cm = std::make_unique<CameraManager>();
    cm->start();

    for (auto const &camera : cm->cameras())
        std::cout << camera->id() << std::endl;

    // Create and aquire camera
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
    camera->acquire(); // Requests lock

    // Configure the camera
    std::unique_ptr<CameraConfiguration> config = camera->generateConfiguration( { StreamRole::Viewfinder } );
    StreamConfiguration &streamConfig = config->at(0);
    std::cout << "Default viewfinder configuration is: " << streamConfig.toString() << std::endl;

    // Change the configuration
    streamConfig.size.width = 640;
    streamConfig.size.height = 480;
    streamConfig.bufferCount = 2;

    // Make sure the new config works, if not, auto-magically create one that should work
    config->validate();
    std::cout << "Validated viewfinder configuration is: " << streamConfig.toString() << std::endl;
    camera->configure(config.get());

    // Allocate FrameBuffers
    FrameBufferAllocator *allocator = new FrameBufferAllocator(camera);
    for (StreamConfiguration &cfg : *config) {
        int ret = allocator->allocate(cfg.stream());
        if (ret < 0) {
            std::cerr << "Can't allocate buffers" << std::endl;
            return -ENOMEM;
        }

        size_t allocated = allocator->buffers(cfg.stream()).size();
        std::cout << "Allocated " << allocated << " buffers for stream" << std::endl;
    }

    Stream *stream = streamConfig.stream();
    const std::vector<std::unique_ptr<FrameBuffer>> &buffers = allocator->buffers(stream);
    std::vector<std::unique_ptr<Request>> requests;

    for (unsigned int i = 0; i < buffers.size(); ++i) {
        std::unique_ptr<Request> request = camera->createRequest();
        if (!request)
        {
            std::cerr << "Can't create request" << std::endl;
            return -ENOMEM;
        }

        const std::unique_ptr<FrameBuffer> &buffer = buffers[i];
        int ret = request->addBuffer(stream, buffer.get());
        if (ret < 0)
        {
            std::cerr << "Can't set buffer for request"
                << std::endl;
            return ret;
        }

        requests.push_back(std::move(request));
    }

    // Event Handling
    // FrameHandler* f_handler = new FrameHandler();
    // using namespace std::placeholders;
    // std::function<void(Request*)> cb_f_handler = std::bind(&FrameHandler::requestComplete, f_handler, _1);
    // camera->requestCompleted.connect(&f_handler, cb_f_handler);

    RequestFlag req_flag = RequestFlag(streamConfig.bufferCount);
    camera->requestCompleted.connect(&req_flag, req_flag.cb_handler);

    // camera->requestCompleted.connect(fastRequestComplete);
    // camera->requestCompleted.connect(requestComplete);

    camera->start();
    for (std::unique_ptr<Request> &request : requests)
        camera->queueRequest(request.get());


    for(size_t i_frame_grabs = 0; i_frame_grabs < 5; i_frame_grabs++)
    {
        for(size_t i_stall = 0; i_stall < 20; i_stall++)
        {
            if(req_flag.ready())
            {
                std::cout << "Exited waiting for frames after " << i_stall << " iters" << std::endl;
                break;
            }
            std::this_thread::sleep_for(100ms);
        }

        if(req_flag.ready())
        {
            std::cout << "We handled the requests!" << std::endl;
            std::vector<Image8b> image_vec;
            for(size_t i_buf = 0; i_buf < buffers.size(); ++i_buf) // Because using unique_ptr it seems like we cannot use "auto buffer : buffers"
            {
                const FrameMetadata &metadata = buffers[i_buf]->metadata();// const FrameMetadata &metadata = buffer->metadata();
        
                std::cout << " seq: " << std::setw(6) << std::setfill('0') << metadata.sequence << " bytesused: ";
        
                unsigned int nplane = 0;
                for (const FrameMetadata::Plane &plane : metadata.planes())
                {
                    std::cout << plane.bytesused;
                    if (++nplane < metadata.planes().size()) std::cout << "/";
                }
        
                // Now give information about the planes
                for (const FrameBuffer::Plane &plane: buffers[i_buf]->planes() )// for (const FrameBuffer::Plane &plane: buffer->planes() )
                {
                    std::cout << "\nThere is at least one plane of length: " << plane.length << std::endl;
                    void *memory = mmap(NULL, plane.length, PROT_READ | PROT_WRITE, MAP_SHARED, plane.fd.get(), 0);
                    libcamera::Span<uint8_t> image_raw(static_cast<uint8_t *>(memory), plane.length);
                    image_vec.emplace_back(480, 640*4, IM_8UC1, (uint8_t*)(image_raw.data()));
                    // Image8b image(480, 640*4, IM_8UC1, (uint8_t*)(image_raw.data())); // This formating works, cuz we have 4 channels, RGBA
                }
            }

            req_flag.reset();

            for (std::unique_ptr<Request> &request : requests)
            {
                request->reuse(Request::ReuseBuffers);
                camera->queueRequest(request.get());
            }
        }
        else
        {
            std::cout << "We were unable to handle the request, exiting" << std::endl;
            break;
        }
    }
    
    // // Wait for X milli-seconds to just see what we get
    // std::this_thread::sleep_for(1000ms);

    std::cout << "here 1" << std::endl;
    camera->stop();
    std::cout << "here 2" << std::endl;
    camera->requestCompleted.disconnect();
    std::cout << "here 3" << std::endl;
    // delete f_handler;
    std::cout << "here 4" << std::endl;
    allocator->free(stream);
    std::cout << "here 5" << std::endl;
    delete allocator;
    camera->release();
    camera.reset();
    cm->stop();

    return 0;
}
