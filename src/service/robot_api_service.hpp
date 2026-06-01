#pragma once
#include <memory>
#include <stdexcept>
#include "corba/corba_client.hpp"

class NotConnectedException : public std::runtime_error {
public:
    NotConnectedException()
        : std::runtime_error("CORBA client is not connected. Call /api/connect first.") {}
};

class RobotApiService {
public:
    RobotApiService() = default;

    void connect(const std::string& ip, int port) {
        client_ = std::make_unique<CorbaClient>(ip, port);
    }

    bool is_connected() const { return client_ != nullptr; }

    // Forwards every call to CorbaClient, raising NotConnectedException if not connected.

    ConnectionStatusResponse  GetConnectionStatus()                                      { return get().GetConnectionStatus(); }
    SimpleResponse            RequestForAuthority()                                      { return get().RequestForAuthority(); }
    SimpleResponse            ReleaseAuthority()                                         { return get().ReleaseAuthority(); }
    SimpleResponse            TurnOnServo()                                              { return get().TurnOnServo(); }
    SimpleResponse            TurnOffServo()                                             { return get().TurnOffServo(); }
    ControllerVarResponse     getControllerVariables()                                   { return get().getControllerVariables(); }
    RobotVariableResponse     getRobotVariables()                                        { return get().getRobotVariables(); }
    VisionVariableResponse    getVisionVariables()                                       { return get().getVisionVariables(); }
    TaskVariableResponse      getTaskVariables()                                         { return get().getTaskVariables(); }
    SimpleResponse            setSpeed(int v)                                            { return get().setSpeed(v); }
    SimpleResponse            setDout(const std::vector<int>& v)                        { return get().setDout(v); }
    SimpleResponse            getDout()                                                  { return get().getDout(); }
    SimpleResponse            setCs(const SaveCSRequest& r)                              { return get().setCs(r); }
    SimpleResponse            setCs2(const SaveCSRequestwithName& r)                    { return get().setCs2(r); }
    SimpleResponse            setCollisionCheck(bool v)                                  { return get().setCollisionCheck(v); }
    SaveCSResponse            getCs(const std::string& cs)                               { return get().getCs(cs); }
    std::vector<SaveCSResponse> getListofCs(const std::vector<std::string>& l)           { return get().getListofCs(l); }
    SimpleResponse            setProductionVolume(int v)                                 { return get().setProductionVolume(v); }
    SimpleResponse            setGlobalVars(const std::string& n, const std::string& v) { return get().setGlobalVars(n, v); }
    SimpleResponse            getGlobalVars(const std::string& n)                       { return get().getGlobalVars(n); }
    SimpleResponse            runCommand(const RunRequest& r)                            { return get().runCommand(r); }
    DriveResponse             drive(const std::string& p, const DriveRequest& r)        { return get().drive(p, r); }
    DriveResponse             driveA(const std::string& p, const DriveRequest& r)       { return get().driveA(p, r); }
    DriveResponse             moveArm(const std::string& p, const MoveRequest& r)       { return get().moveArm(p, r); }
    SimpleResponse            waitMotion(const std::string& p, int q)                   { return get().waitMotion(p, q); }
    SimpleResponse            halt()                                                     { return get().halt(); }
    SimpleResponse            hold()                                                     { return get().hold(); }
    SimpleResponse            unHold()                                                   { return get().unHold(); }
    GrabbedImageData          grabImage(const GrabImageRequest& r)                       { return get().grabImage(r); }
    SimpleResponse            start()                                                    { return get().start(); }
    SimpleResponse            stop()                                                     { return get().stop(); }
    SimpleResponse            resume(const std::string& mode)                            { return get().resume(mode); }
    SimpleResponse            getTaskNames()                                             { return get().getTaskNames(); }
    SimpleResponse            setTask(const std::string& t)                              { return get().setTask(t); }
    SimpleResponse            resetProdRecord()                                          { return get().resetProdRecord(); }

private:
    std::unique_ptr<CorbaClient> client_;

    CorbaClient& get() {
        if (!client_) throw NotConnectedException();
        return *client_;
    }
};
