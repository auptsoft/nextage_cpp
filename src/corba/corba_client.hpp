#pragma once
#include <omniORB4/CORBA.h>
#include <string>
#include <vector>
#include <memory>

// Generated IDL stubs (produced by omniidl at build time)
#include "NxApi.hh"

#include "any_helper.hpp"
#include "dto/requests.hpp"
#include "dto/responses.hpp"

class CorbaClient {
public:
    CorbaClient(const std::string& ip, int port);
    ~CorbaClient();

    // Connection
    ConnectionStatusResponse GetConnectionStatus();

    // Authority
    SimpleResponse RequestForAuthority();
    SimpleResponse ReleaseAuthority();

    // Servo
    SimpleResponse TurnOnServo();
    SimpleResponse TurnOffServo();

    // Variable reads
    ControllerVarResponse  getControllerVariables();
    RobotVariableResponse  getRobotVariables();
    VisionVariableResponse getVisionVariables();
    TaskVariableResponse   getTaskVariables();

    // Controller variable writes
    SimpleResponse setSpeed(int value);
    SimpleResponse setDout(const std::vector<int>& value);
    SimpleResponse getDout();
    SimpleResponse setCs(const SaveCSRequest& req);
    SimpleResponse setCs2(const SaveCSRequestwithName& req);
    SimpleResponse setCollisionCheck(bool value);
    SaveCSResponse getCs(const std::string& csNumber);
    std::vector<SaveCSResponse> getListofCs(const std::vector<std::string>& csList);

    // Task variable writes
    SimpleResponse setProductionVolume(int value);
    SimpleResponse setGlobalVars(const std::string& variableName, const std::string& value);
    SimpleResponse getGlobalVars(const std::string& varName);

    // Controller extended commands
    SimpleResponse runCommand(const RunRequest& req);

    // Robot motion commands
    DriveResponse  drive(const std::string& part, const DriveRequest& req);
    DriveResponse  driveA(const std::string& part, const DriveRequest& req);
    DriveResponse  moveArm(const std::string& part, const MoveRequest& req);
    SimpleResponse waitMotion(const std::string& part, int queueSize);
    SimpleResponse halt();
    SimpleResponse hold();
    SimpleResponse unHold();

    // Vision
    GrabbedImageData grabImage(const GrabImageRequest& req);

    // Task commands
    SimpleResponse start();
    SimpleResponse stop();
    SimpleResponse resume(const std::string& mode);
    SimpleResponse getTaskNames();
    SimpleResponse setTask(const std::string& taskName);
    SimpleResponse resetProdRecord();

private:
    std::string ip_;
    int port_;

    CORBA::ORB_var          orb_;
    NxApi::RootNxController_var root_controller_;
    NxApi::NxController_var     controller_;

    // Helper: execute a command on controller directly
    CORBA::Any ctrl_execute(const std::wstring& cmd, const CORBA::Any& arg);

    // Helper: get NxObject then execute
    CORBA::Any obj_execute(const std::wstring& type, const std::wstring& name,
                           const std::wstring& cmd, const CORBA::Any& arg);

    // Helper: get variable then execute
    CORBA::Any var_execute(const std::wstring& varName,
                           const std::wstring& cmd, const CORBA::Any& arg);

    // Helper: get variable on a sub-object then execute
    CORBA::Any subobj_var_execute(const std::wstring& objType, const std::wstring& objName,
                                   const std::wstring& varName,
                                   const std::wstring& cmd, const CORBA::Any& arg);

    SimpleResponse simple_status(CORBA::Long status, const std::string& ok_msg, const std::string& fail_msg);
};
