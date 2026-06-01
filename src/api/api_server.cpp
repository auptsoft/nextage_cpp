#include "api_server.hpp"
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <stdexcept>
#include <iostream>

// ---- Constructor ----

ApiServer::ApiServer(RobotApiService& service, const std::string& www_root)
    : service_(service), www_root_(www_root)
{
    register_routes();
}

// ---- Run / stop ----

void ApiServer::run(const std::string& host, int port) {
    svr_.new_task_queue = [] { return new httplib::ThreadPool(4); };
    svr_.listen(host.c_str(), port);
}

void ApiServer::stop() { svr_.stop(); }

// ---- CORS helper ----

void ApiServer::add_cors(httplib::Response& res) {
    res.set_header("Access-Control-Allow-Origin",  "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
}

// ---- Error-catching wrapper ----

void ApiServer::safe_handle(httplib::Response& res, std::function<void()> fn) {
    try {
        fn();
    } catch (const NotConnectedException& e) {
        res.status = 503;
        res.set_content(std::string("{\"error\":\"") + e.what() + "\"}", "application/json");
    } catch (const std::exception& e) {
        res.status = 500;
        res.set_content(std::string("{\"error\":\"") + e.what() + "\"}", "application/json");
    }
}

// ---- Route registration ----

void ApiServer::register_routes() {

    // CORS preflight for all routes
    svr_.Options(".*", [this](const httplib::Request&, httplib::Response& res) {
        add_cors(res);
        res.status = 204;
    });

    // ------------------------------------------------------------------ //
    //  Connection
    // ------------------------------------------------------------------ //

    svr_.Post("/api/connect", [this](const httplib::Request& req, httplib::Response& res) {


        add_cors(res);
        
        safe_handle(res, [&] {
            std::string ip = req.get_param_value("ip_address");
            int port = req.has_param("port") ? std::stoi(req.get_param_value("port")) : 2809;
            std::cout << "Connecting. \n";
            service_.connect(ip, port);
            res.set_content("{\"message\":\"Connected successfully.\",\"isSuccessful\":true}", "application/json");
        });
    });

    svr_.Get("/get_connection_status", [this](const httplib::Request&, httplib::Response& res) {
        add_cors(res);
        safe_handle(res, [&] {
            json_response(res, service_.GetConnectionStatus());
        });
    });

    svr_.Post("/api/request_for_authority", [this](const httplib::Request&, httplib::Response& res) {
        add_cors(res);
        safe_handle(res, [&] { json_response(res, service_.RequestForAuthority()); });
    });

    svr_.Post("/api/release_authority", [this](const httplib::Request&, httplib::Response& res) {
        add_cors(res);
        safe_handle(res, [&] { json_response(res, service_.ReleaseAuthority()); });
    });

    // ------------------------------------------------------------------ //
    //  Servo
    // ------------------------------------------------------------------ //

    svr_.Post("/api/turn_on_servo", [this](const httplib::Request&, httplib::Response& res) {
        add_cors(res);
        safe_handle(res, [&] { json_response(res, service_.TurnOnServo()); });
    });

    svr_.Post("/api/turn_off_servo", [this](const httplib::Request&, httplib::Response& res) {
        add_cors(res);
        safe_handle(res, [&] { json_response(res, service_.TurnOffServo()); });
    });

    // ------------------------------------------------------------------ //
    //  Variable reads
    // ------------------------------------------------------------------ //

    svr_.Get("/api/get_controller_variables", [this](const httplib::Request&, httplib::Response& res) {
        add_cors(res);
        safe_handle(res, [&] { json_response(res, service_.getControllerVariables()); });
    });

    svr_.Get("/api/get_robot_variables", [this](const httplib::Request&, httplib::Response& res) {
        add_cors(res);
        safe_handle(res, [&] { json_response(res, service_.getRobotVariables()); });
    });

    svr_.Get("/api/get_vision_variables", [this](const httplib::Request&, httplib::Response& res) {
        add_cors(res);
        safe_handle(res, [&] { json_response(res, service_.getVisionVariables()); });
    });

    svr_.Get("/api/get_task_variables", [this](const httplib::Request&, httplib::Response& res) {
        add_cors(res);
        safe_handle(res, [&] { json_response(res, service_.getTaskVariables()); });
    });

    // ------------------------------------------------------------------ //
    //  Controller variable writes
    // ------------------------------------------------------------------ //

    svr_.Post("/api/set_speed", [this](const httplib::Request& req, httplib::Response& res) {
        add_cors(res);
        safe_handle(res, [&] {
            SetSpeedRequest r;
            int speed = req.has_param("value") ? std::stoi(req.get_param_value("value")) : 20;
            // if (!parse_body(req, res, r)) return;
            json_response(res, service_.setSpeed(speed));
        });
    });

    svr_.Post("/api/set_dout", [this](const httplib::Request& req, httplib::Response& res) {
        add_cors(res);
        safe_handle(res, [&] {
            SetDoutRequest r;
            if (!parse_body(req, res, r)) return;
            json_response(res, service_.setDout(r.value));
        });
    });

    svr_.Get("/api/get_dout", [this](const httplib::Request&, httplib::Response& res) {
        add_cors(res);
        safe_handle(res, [&] { json_response(res, service_.getDout()); });
    });

    svr_.Post("/api/set_cs", [this](const httplib::Request& req, httplib::Response& res) {
        add_cors(res);
        safe_handle(res, [&] {
            SaveCSRequest r;
            if (!parse_body(req, res, r)) return;
            json_response(res, service_.setCs(r));
        });
    });

    svr_.Post("/api/set_cs2", [this](const httplib::Request& req, httplib::Response& res) {
        add_cors(res);
        safe_handle(res, [&] {
            SaveCSRequestwithName r;
            if (!parse_body(req, res, r)) return;
            json_response(res, service_.setCs2(r));
        });
    });

    svr_.Get("/api/get_cs/", [this](const httplib::Request& req, httplib::Response& res) {
        add_cors(res);
        safe_handle(res, [&] {
            std::string csNumber = req.get_param_value("csNumber");
            json_response(res, service_.getCs(csNumber));
        });
    });

    svr_.Post("/api/set_collision_check", [this](const httplib::Request& req, httplib::Response& res) {
        add_cors(res);
        safe_handle(res, [&] {
            SetCollisionCheckRequest r;
            if (!parse_body(req, res, r)) return;
            json_response(res, service_.setCollisionCheck(r.value));
        });
    });

    // ------------------------------------------------------------------ //
    //  Task variable writes
    // ------------------------------------------------------------------ //

    svr_.Post("/api/set_production_volume", [this](const httplib::Request& req, httplib::Response& res) {
        add_cors(res);
        safe_handle(res, [&] {
            SetProductionVolumeRequest r;
            if (!parse_body(req, res, r)) return;
            json_response(res, service_.setProductionVolume(r.value));
        });
    });

    svr_.Post("/api/set_global_vars", [this](const httplib::Request& req, httplib::Response& res) {
        add_cors(res);
        safe_handle(res, [&] {
            SetGlobalVarsRequest r;
            if (!parse_body(req, res, r)) return;
            json_response(res, service_.setGlobalVars(r.name, r.value));
        });
    });

    svr_.Get("/api/get_global_vars", [this](const httplib::Request& req, httplib::Response& res) {
        add_cors(res);
        safe_handle(res, [&] {
            std::string variableName = req.get_param_value("variableName");
            json_response(res, service_.getGlobalVars(variableName));
        });
    });

    // ------------------------------------------------------------------ //
    //  Controller extended commands
    // ------------------------------------------------------------------ //

    svr_.Post("/api/run_command", [this](const httplib::Request& req, httplib::Response& res) {
        add_cors(res);
        safe_handle(res, [&] {
            RunRequest r;
            if (!parse_body(req, res, r)) return;
            json_response(res, service_.runCommand(r));
        });
    });

    // ------------------------------------------------------------------ //
    //  Robot motion
    // ------------------------------------------------------------------ //

    svr_.Post("/api/drive_joint", [this](const httplib::Request& req, httplib::Response& res) {
        add_cors(res);
        safe_handle(res, [&] {
            auto body = nlohmann::json::parse(req.body);
            std::string part = body.at("part").get<std::string>();
            DriveRequest dr  = body.at("request").get<DriveRequest>();
            json_response(res, service_.drive(part, dr));
        });
    });

    svr_.Post("/api/drive_jointA", [this](const httplib::Request& req, httplib::Response& res) {
        add_cors(res);
        safe_handle(res, [&] {
            auto body = nlohmann::json::parse(req.body);
            std::string part = body.at("part").get<std::string>();
            DriveRequest dr  = body.at("request").get<DriveRequest>();
            json_response(res, service_.driveA(part, dr));
        });
    });

    svr_.Post("/api/move_to_point", [this](const httplib::Request& req, httplib::Response& res) {
        add_cors(res);
        safe_handle(res, [&] {
            auto body = nlohmann::json::parse(req.body);
            std::string part = body.at("part").get<std::string>();
            MoveRequest mr   = body.at("request").get<MoveRequest>();
            json_response(res, service_.moveArm(part, mr));
        });
    });

    svr_.Post("/api/wait_motion", [this](const httplib::Request& req, httplib::Response& res) {
        add_cors(res);
        safe_handle(res, [&] {
            WaitMotionRequest r;
            if (!parse_body(req, res, r)) return;
            json_response(res, service_.waitMotion(r.part, r.queue));
        });
    });

    svr_.Post("/api/halt", [this](const httplib::Request&, httplib::Response& res) {
        add_cors(res);
        safe_handle(res, [&] { json_response(res, service_.halt()); });
    });

    svr_.Post("/api/hold", [this](const httplib::Request&, httplib::Response& res) {
        add_cors(res);
        safe_handle(res, [&] { json_response(res, service_.hold()); });
    });

    svr_.Post("/api/unHold", [this](const httplib::Request&, httplib::Response& res) {
        add_cors(res);
        safe_handle(res, [&] { json_response(res, service_.unHold()); });
    });

    // ------------------------------------------------------------------ //
    //  Vision
    // ------------------------------------------------------------------ //

    svr_.Post("/api/grab_image", [this](const httplib::Request& req, httplib::Response& res) {
        add_cors(res);
        safe_handle(res, [&] {
            GrabImageRequest r;
            if (!parse_body(req, res, r)) return;
            json_response(res, service_.grabImage(r));
        });
    });

    // Returns the image file directly (equivalent to FastAPI's FileResponse)
    svr_.Get("/api/grab_image_url", [this](const httplib::Request& req, httplib::Response& res) {
        add_cors(res);
        safe_handle(res, [&] {
            GrabImageRequest r;
            r.cameraNumber      = std::stoi(req.get_param_value("cameraNumber"));
            r.view              = req.get_param_value("view") == "true";
            r.returnImage       = req.get_param_value("returnImage") == "true";
            r.distortionCorrect = req.get_param_value("distortionCorrect") == "true";
            r.exposure          = std::stod(req.get_param_value("exposure"));
            r.format            = std::stoi(req.get_param_value("format"));

            GrabbedImageData data = service_.grabImage(r);

            std::ifstream file(data.imageFilePath, std::ios::binary);
            if (!file) {
                res.status = 404;
                res.set_content("{\"error\":\"Image file not found\"}", "application/json");
                return;
            }
            std::string content((std::istreambuf_iterator<char>(file)),
                                  std::istreambuf_iterator<char>());
            std::string mime = (r.format == 0) ? "image/bmp" : "image/jpeg";
            res.set_content(content, mime.c_str());
        });
    });

    // ------------------------------------------------------------------ //
    //  Task commands
    // ------------------------------------------------------------------ //

    svr_.Post("/api/start", [this](const httplib::Request&, httplib::Response& res) {
        add_cors(res);
        safe_handle(res, [&] { json_response(res, service_.start()); });
    });

    svr_.Post("/api/stop", [this](const httplib::Request&, httplib::Response& res) {
        add_cors(res);
        safe_handle(res, [&] { json_response(res, service_.stop()); });
    });

    svr_.Post("/api/resume", [this](const httplib::Request& req, httplib::Response& res) {
        add_cors(res);
        safe_handle(res, [&] {
            ResumeRequest r;
            if (!parse_body(req, res, r)) return;
            json_response(res, service_.resume(r.mode));
        });
    });

    svr_.Get("/api/get_task_names", [this](const httplib::Request&, httplib::Response& res) {
        add_cors(res);
        safe_handle(res, [&] { json_response(res, service_.getTaskNames()); });
    });

    svr_.Post("/api/set_task", [this](const httplib::Request& req, httplib::Response& res) {
        add_cors(res);
        safe_handle(res, [&] {
            SetTaskRequest r;
            if (!parse_body(req, res, r)) return;
            json_response(res, service_.setTask(r.taskName));
        });
    });

    svr_.Post("/api/reset_prod_record", [this](const httplib::Request&, httplib::Response& res) {
        add_cors(res);
        safe_handle(res, [&] { json_response(res, service_.resetProdRecord()); });
    });

    // ------------------------------------------------------------------ //
    //  Static file serving (Vue.js SPA)
    // ------------------------------------------------------------------ //

    if (!www_root_.empty() && std::filesystem::exists(www_root_)) {
        // Serve /assets/*, /images/*, /favicon.ico directly
        svr_.set_mount_point("/", www_root_.c_str());
    }

    // SPA fallback: any unmatched GET returns index.html
    svr_.set_error_handler([this](const httplib::Request& req, httplib::Response& res) {
        // API paths must never receive HTML — preserve JSON error or return 404
        if (req.path.rfind("/api", 0) == 0) {
            if (res.body.empty())
                res.set_content("{\"error\":\"Not found\"}", "application/json");
            return;
        }
        if (req.method == "GET" && !www_root_.empty()) {
            std::string index = www_root_ + "/index.html";
            std::ifstream f(index, std::ios::binary);
            if (f) {
                std::string html((std::istreambuf_iterator<char>(f)),
                                  std::istreambuf_iterator<char>());
                res.set_content(html, "text/html");
                res.status = 200;
                return;
            }
        }
        res.set_content("{\"error\":\"Not found\"}", "application/json");
    });
}
