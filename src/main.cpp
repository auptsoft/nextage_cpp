#include <iostream>
#include <string>
#include <filesystem>
#include "service/robot_api_service.hpp"
#include "api/api_server.hpp"

static void print_usage(const char* prog) {
    std::cerr << "Usage: " << prog << " [--host <host>] [--port <port>] [--www <path>]\n"
              << "  --host   Bind address  (default: 0.0.0.0)\n"
              << "  --port   HTTP port     (default: 8000)\n"
              << "  --www    Path to www/  (default: ../NextageAPI_Task/www)\n";
}

int main(int argc, char* argv[]) {
    std::string host     = "0.0.0.0";
    int         port     = 8000;
    std::string www_root = "../NextageAPI_Task/www";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "--host") && i + 1 < argc)      host     = argv[++i];
        else if ((arg == "--port") && i + 1 < argc)  port     = std::stoi(argv[++i]);
        else if ((arg == "--www")  && i + 1 < argc)  www_root = argv[++i];
        else if (arg == "--help")  { print_usage(argv[0]); return 0; }
        else { std::cerr << "Unknown argument: " << arg << "\n"; print_usage(argv[0]); return 1; }
    }

    // Resolve www path relative to the executable
    if (!std::filesystem::exists(www_root)) {
        std::cerr << "[warn] www root not found at: " << www_root << " — static files will not be served\n";
        www_root = "";
    }

    RobotApiService service;
    ApiServer       server(service, www_root);

    std::cout << "Nextage API server starting on http://" << host << ":" << port << "\n";
    if (!www_root.empty())
        std::cout << "Serving frontend from: " << www_root << "\n";
    std::cout << "Call POST /api/connect first to establish a CORBA connection.\n";

    server.run(host, port);
    return 0;
}
