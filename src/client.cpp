#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

#include <iostream>
#include "opencv2/core/mat.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include "proto/x3.pb.h"

typedef websocketpp::client<websocketpp::config::asio_client> client;

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

// pull out the type of messages sent by our config
typedef websocketpp::config::asio_client::message_type::ptr message_ptr;

// Create a client endpoint
client ws_c;
websocketpp::connection_hdl hdl_server_;
std::thread send_task_;
bool is_running_ = true;
bool is_connected_ = false;
x3::Capture capture_;

// 信号处理函数  
void signalHandler(int signum) {  
    std::cout << "catch signal " << signum << std::endl;
    ws_c.stop_listening(); 
    ws_c.stop();      
}  

// 连接打开时的回调函数  
void on_open(websocketpp::connection_hdl hdl, client* ws_c) {  
    std::cout << "Connection opened" << std::endl;
    hdl_server_ = hdl;  
    is_connected_ = true;
}  

// This message handler will be invoked once for each incoming message. It
// prints the message and then sends a copy of the message back to the server.
void on_message(client* ws_c, websocketpp::connection_hdl hdl, message_ptr msg) {
    auto opCode = msg->get_opcode();
    const auto& message = msg->get_payload();
    std::cout << "on_message called with hdl: " << hdl.lock().get()
              << ", opCode: " << opCode
              << ", message len: " << message.size()
              << std::endl;
    if (opCode == websocketpp::frame::opcode::value::TEXT) {
        std::cout << "recved message: " << message
                << std::endl;
    } else if (opCode == websocketpp::frame::opcode::value::BINARY) {
        // 反序列化感知结果
        x3::SmartFrameMessage smart;
        if (!smart.ParseFromString(message)) {
          std::cout << "parse protobuf failed\n";
          return;
        }
        std::stringstream ss;
        ss << "recved smart message ts: " << smart.timestamp_()
          << ", error_code: " << smart.error_code_()
          << ", target size: " << smart.targets_().size();
        for (const auto& target : smart.targets_()) {
          ss << "\n type: " << target.type_();
          for (const auto& box : target.boxes_()) {
            ss << ", box type: " << box.type_()
              << ", top_left: " << box.top_left_().x_() << " " << box.top_left_().y_()
              << ", bottom_right: " << box.bottom_right_().x_() << " " << box.bottom_right_().y_()
              << ", score: " << box.score();
          }
          for (const auto& attribute : target.attributes_()) {
            ss << ", " << attribute.type_() << ": " << attribute.value_();
          }
        }
        std::cout << ss.str() << std::endl;
    }
}


bool processImage(const std::string &image_source,
                  const std::string &image_format) {
  if (access(image_source.c_str(), R_OK) == -1) {
    printf(
                 "Image: %s not exist!",
                 image_source.c_str());
    return false;
  }
  cv::Mat bgr_mat;

  // 获取图片
  if (image_format == "jpeg") {
    bgr_mat = cv::imread(image_source, cv::IMREAD_COLOR);
  }

  // 使用opencv的imencode接口将mat转成vector，获取图片size
  std::vector<int> param;
  std::vector<uint8_t> jpeg_data;
  imencode(".jpg", bgr_mat, jpeg_data, param);
  
  // 获取当前时间，转成纳秒时间戳
  capture_.set_timestamp_(std::chrono::system_clock::now().time_since_epoch().count());
  auto image = capture_.mutable_img_();
  image->set_buf_((const char *)jpeg_data.data(), jpeg_data.size());
  image->set_type_("jpeg");
  image->set_width_(bgr_mat.cols);
  image->set_height_(bgr_mat.rows);

  return true;
}

int main(int argc, char* argv[]) {
  if (argc < 3) {
    std::cout << "Usage: ./tros_websocket_client <url> <img file>" << std::endl;
    return -1;
  }

  // 注册信号SIGINT和对应的处理函数  
  signal(SIGINT, signalHandler);    
  // std::string uri = "http://localhost:8081";
  std::string uri = argv[1];
  std::string img_file = argv[2];
  
  std::cout << "Connecting to " << uri << " and send img " << img_file << std::endl;
  if (!processImage(img_file, "jpeg")) {
    std::cout << "process image failed" << std::endl;
    return -1;
  }
  std::cout << "process image success" << std::endl;

  try {
    // Set logging to be pretty verbose (everything except message payloads)
    ws_c.set_access_channels(websocketpp::log::alevel::all);
    ws_c.clear_access_channels(websocketpp::log::alevel::frame_payload);

    // Initialize ASIO
    ws_c.init_asio();

    // 设置连接打开和消息接收的回调函数  
    ws_c.set_open_handler(bind(&on_open, ::_1, &ws_c));  
    // Register our message handler
    ws_c.set_message_handler(bind(&on_message,&ws_c,::_1,::_2));

    websocketpp::lib::error_code ec;
    client::connection_ptr con = ws_c.get_connection(uri, ec);
    if (ec) {
        std::cout << "could not create connection because: " << ec.message() << std::endl;
        return 0;
    }

    // Note that connect here only requests a connection. No network messages are
    // exchanged until the event loop starts running in the next line.
    ws_c.connect(con);

    send_task_ = std::thread([&, &ws_c]() {
        std::cout << "send task is running" << std::endl;
        while (is_running_) {
            if (!is_connected_) {
                std::cout << "waiting for connection" << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                continue;
            }

            std::cout << "send image\n\n";
            std::string proto_send;
            capture_.set_timestamp_(std::chrono::system_clock::now().time_since_epoch().count());
            capture_.SerializeToString(&proto_send);
            ws_c.send(hdl_server_, reinterpret_cast<void *>(proto_send.data()), proto_send.size(),
            websocketpp::frame::opcode::binary);
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    });

    // Start the ASIO io_service run loop
    // this will cause a single connection to be made to the server. ws_c.run()
    // will exit when this connection is closed.
    ws_c.run();
  } catch (websocketpp::exception const & e) {
    is_running_ = false;
    ws_c.stop_listening(); 
    ws_c.stop();      
    std::cout << "exception: " << e.what() << std::endl;
  }
}
