#include <websocketpp/config/asio_no_tls.hpp>

#include <websocketpp/server.hpp>

#include <iostream>
#include <csignal>  

typedef websocketpp::server<websocketpp::config::asio> server;

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

// pull out the type of messages sent by our config
typedef server::message_ptr message_ptr;

server echo_server;

// 信号处理函数  
void signalHandler(int signum) {  
    std::cout << "catch signal " << signum << std::endl;
    echo_server.stop_listening(); 
    echo_server.stop();      
}  

// Define a callback to handle incoming messages
void on_message(server* s, websocketpp::connection_hdl hdl, message_ptr msg) {
    std::cout << "on_message called with hdl: " << hdl.lock().get()
              << " and message: " << msg->get_payload()
              << std::endl;

    // check for a special command to instruct the server to stop listening so
    // it can be cleanly exited.
    if (msg->get_payload() == "stop-listening") {
        s->stop_listening();
        return;
    }

    try {
        s->send(hdl, msg->get_payload(), msg->get_opcode());
    } catch (websocketpp::exception const & e) {
        std::cout << "Echo failed because: "
                  << "(" << e.what() << ")" << std::endl;
    }
}

void shutdownWebsocket() {
    std::cout << "Shutting down websocket" << std::endl;
    echo_server.stop_listening(); 
    echo_server.stop();      
}

int main() {

    // 注册信号SIGINT和对应的处理函数  
    signal(SIGINT, signalHandler);    

    // Create a server endpoint
    try {
        // Set logging settings
        echo_server.set_access_channels(websocketpp::log::alevel::all);
        echo_server.clear_access_channels(websocketpp::log::alevel::frame_payload);

        // Initialize Asio
        echo_server.init_asio();

        // Register our message handler
        echo_server.set_message_handler(bind(&on_message,&echo_server,::_1,::_2));

        // Listen on port 9002
        echo_server.listen(9002);

        // Start the server accept loop
        echo_server.start_accept();

        // Start the ASIO io_service run loop
        echo_server.run();
    } catch (websocketpp::exception const & e) {
        std::cout << "exception: " << e.what() << std::endl;
    } catch (...) {
        std::cout << "other exception" << std::endl;
    }
    
    // echo_server.stop();      
    
    // std::atexit(shutdownWebsocket);  

    std::cout << "exit..." << std::endl;
}
