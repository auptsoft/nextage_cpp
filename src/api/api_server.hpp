#pragma once
#include <httplib.h>
#include <string>
#include "service/robot_api_service.hpp"

class ApiServer {
public:
    explicit ApiServer(RobotApiService& service, const std::string& www_root = "");

    // Blocks until the server stops.
    void run(const std::string& host, int port);

    void stop();

private:
    RobotApiService& service_;
    std::string      www_root_;
    httplib::Server  svr_;

    void register_routes();
    void add_cors(httplib::Response& res);

    // Convenience: serialise any to_json-able value and set on response
    template<typename T>
    void json_response(httplib::Response& res, const T& value) {
        nlohmann::json j = value;
        res.set_content(j.dump(), "application/json");
    }

    // Parse JSON body into T; returns false and sets 400 on failure
    template<typename T>
    bool parse_body(const httplib::Request& req, httplib::Response& res, T& out) {
        try {
            out = nlohmann::json::parse(req.body).get<T>();
            return true;
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(std::string("{\"error\":\"Bad request: ") + e.what() + "\"}", "application/json");
            return false;
        }
    }

    // Wrap a handler that may throw; sets 500 on exception
    void safe_handle(httplib::Response& res, std::function<void()> fn);
};
