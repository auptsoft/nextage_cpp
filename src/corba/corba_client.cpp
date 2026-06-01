#include "corba_client.hpp"
#include <omniORB4/CORBA.h>
#include <sstream>
#include <stdexcept>
#include <ctime>
#include <fstream>
#include <filesystem>
#include <codecvt>
#include <locale>
#include <nlohmann/json.hpp>

// ---- Narrow string → wide string ----
static std::wstring ws(const std::string& s) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
    return conv.from_bytes(s);
}

// ---- Constructor / destructor ----

CorbaClient::CorbaClient(const std::string& ip, int port)
    : ip_(ip), port_(port)
{
    const char* argv[] = {
        "-ORBdefaultWCharCodeSet", "UTF-16",
        "-ORBgiopMaxMsgSize",      "104857600"
    };
    int argc = 4;
    orb_ = CORBA::ORB_init(argc, const_cast<char**>(argv));

    std::ostringstream ref;
    ref << "corbaloc:iiop:1.2@" << ip_ << ":" << port_ << "/RootControllerApi";

    CORBA::Object_var obj = orb_->string_to_object(ref.str().c_str());
    root_controller_ = NxApi::RootNxController::_narrow(obj);
    if (CORBA::is_nil(root_controller_))
        throw std::runtime_error("Failed to narrow RootNxController");

    controller_ = root_controller_->GetNxController(L"", L"");
    if (CORBA::is_nil(controller_))
        throw std::runtime_error("Failed to get NxController");
}

CorbaClient::~CorbaClient() {
    try {
        orb_->destroy();
    } catch (...) {}
}

// ---- Private helper implementations ----

CORBA::Any CorbaClient::ctrl_execute(const std::wstring& cmd, const CORBA::Any& arg) {
    return controller_->Execute(cmd.c_str(), arg);
}

CORBA::Any CorbaClient::obj_execute(const std::wstring& type, const std::wstring& name,
                                     const std::wstring& cmd, const CORBA::Any& arg) {
    NxApi::NxObject_var obj = controller_->GetNxObject(type.c_str(), name.c_str());
    return obj->Execute(cmd.c_str(), arg);
}

CORBA::Any CorbaClient::var_execute(const std::wstring& varName,
                                     const std::wstring& cmd, const CORBA::Any& arg) {
    NxApi::NxObject_var var = controller_->GetVariable(varName.c_str(), L"");
    return var->Execute(cmd.c_str(), arg);
}

CORBA::Any CorbaClient::subobj_var_execute(const std::wstring& objType, const std::wstring& objName,
                                            const std::wstring& varName,
                                            const std::wstring& cmd, const CORBA::Any& arg) {
    NxApi::NxObject_var obj = controller_->GetNxObject(objType.c_str(), objName.c_str());
    NxApi::NxObject_var var = obj->GetVariable(varName.c_str(), L"");
    return var->Execute(cmd.c_str(), arg);
}

SimpleResponse CorbaClient::simple_status(CORBA::Long status,
                                           const std::string& ok_msg,
                                           const std::string& fail_msg) {
    return {status == 0 ? ok_msg : fail_msg, static_cast<int>(status), status == 0, nullptr};
}

// ---- Connection ----

ConnectionStatusResponse CorbaClient::GetConnectionStatus() {
    CORBA::Any result = var_execute(L"@CONNECT_STATUS", L"GetValues", AnyHelper::make_null());
    auto elems = AnyHelper::extract_seq(result);

    // elems[0] is the status entry — index into it
    auto inner = AnyHelper::extract_seq(elems[0]);
    CORBA::Boolean ok = AnyHelper::extract_bool(inner[0]);

    if (ok) {
        return {true, "", "", "", ""};
    }

    // Failure: inner[1] = unconnected plugins, inner[2] = nested error info
    nlohmann::json unconnected = AnyHelper::to_json(inner[1]);
    auto error_info = AnyHelper::extract_seq(inner[2]);

    return {
        false,
        unconnected.dump(),
        AnyHelper::to_json(error_info[0]).dump(),
        AnyHelper::to_json(error_info[1]).dump(),
        AnyHelper::to_json(error_info[2]).dump()
    };
}

// ---- Authority ----

SimpleResponse CorbaClient::RequestForAuthority() {
    CORBA::Any result = ctrl_execute(L"GetAuthority", AnyHelper::make_null());
    auto elems = AnyHelper::extract_seq(result);
    CORBA::Long code = AnyHelper::extract_long(elems[0]);
    std::wstring msg  = AnyHelper::extract_wstring(elems[1]);
    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
    return {conv.to_bytes(msg), static_cast<int>(code), code == 0, nullptr};
}

SimpleResponse CorbaClient::ReleaseAuthority() {
    CORBA::Any result = ctrl_execute(L"ReleaseAuthority", AnyHelper::make_null());
    CORBA::Long status = AnyHelper::extract_long(result);
    return simple_status(status, "Authority released", "Failed to release authority");
}

// ---- Servo ----

SimpleResponse CorbaClient::TurnOnServo() {
    CORBA::Any result = obj_execute(L"Robot", L"All", L"ServoOn", AnyHelper::make_null());
    return simple_status(AnyHelper::extract_long(result), "Servo turned on", "Failed to turn on servo");
}

SimpleResponse CorbaClient::TurnOffServo() {
    CORBA::Any result = obj_execute(L"Robot", L"All", L"ServoOff", AnyHelper::make_null());
    return simple_status(AnyHelper::extract_long(result), "Servo turned off", "Failed to turn off servo");
}

// ---- Variable reads ----

ControllerVarResponse CorbaClient::getControllerVariables() {
    const std::wstring names =
        L"@ID @CONTROLLER_NAMES @AUTO_MODE @EXT_CONTROL @API_CONTROL "
        L"@SPEED @DIN @DOUT "
        L"@CS/0 @CS/1 @CS/2 @CS/3 @CS/4 @CS/5 @CS/6 @CS/7 @CS/8 @CS/9 "
        L"@EMERGENCY_STOP @EMERGENCY_STOP_CODE @HOLD_RESTART @COLLISION_CHECK "
        L"@TOTAL_OPERATING_TIME @CONNECT_STATUS @PLUGIN_LIST @IF_VERSION @SYSTEM_VERSION";

    NxApi::NxObject_var vars = controller_->GetVariable(names.c_str(), L"");
    CORBA::Any result = vars->Execute(L"GetValues", AnyHelper::make_null());
    auto v = AnyHelper::extract_seq(result);

    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
    ControllerVarResponse r;
    r.id               = conv.to_bytes(AnyHelper::extract_wstring(v[0]));
    r.controllerNames  = AnyHelper::to_json(v[1]);
    r.autoMode         = AnyHelper::extract_bool(v[2]);
    r.extControl       = AnyHelper::extract_bool(v[3]);
    r.apiControl       = conv.to_bytes(AnyHelper::extract_wstring(v[4]));
    r.speed            = static_cast<int>(AnyHelper::extract_long(v[5]));
    r.dIn              = AnyHelper::to_json(v[6]);
    r.dOut             = AnyHelper::to_json(v[7]);
    r.CS0=AnyHelper::to_json(v[8]);  r.CS1=AnyHelper::to_json(v[9]);
    r.CS2=AnyHelper::to_json(v[10]); r.CS3=AnyHelper::to_json(v[11]);
    r.CS4=AnyHelper::to_json(v[12]); r.CS5=AnyHelper::to_json(v[13]);
    r.CS6=AnyHelper::to_json(v[14]); r.CS7=AnyHelper::to_json(v[15]);
    r.CS8=AnyHelper::to_json(v[16]); r.CS9=AnyHelper::to_json(v[17]);
    r.emergencyStop     = AnyHelper::extract_bool(v[18]);
    r.emergencyStopCode = static_cast<int>(AnyHelper::extract_long(v[19]));
    r.holdRestart       = AnyHelper::extract_bool(v[20]);
    r.collisionCheck    = AnyHelper::extract_bool(v[21]);
    r.totalOperatingTime = static_cast<int>(AnyHelper::extract_long(v[22]));
    r.connectStatus     = AnyHelper::to_json(v[23]);
    r.pluginList        = AnyHelper::to_json(v[24]);
    r.IFVersion         = conv.to_bytes(AnyHelper::extract_wstring(v[25]));
    r.systemVersion     = conv.to_bytes(AnyHelper::extract_wstring(v[26]));
    return r;
}

RobotVariableResponse CorbaClient::getRobotVariables() {
    const std::wstring names =
        L"@SERVO_ON @SERVO_STATUS @JOINT_ANGLE @ACTUAL_JOINT_ANGLE "
        L"@BUSY_STATUS @PAUSE @PAUSE_CODE @TOTAL_OPERATING_TIME";

    NxApi::NxObject_var robot = controller_->GetNxObject(L"Robot", L"All");
    NxApi::NxObject_var vars  = robot->GetVariable(names.c_str(), L"");
    CORBA::Any result = vars->Execute(L"GetValues", AnyHelper::make_null());
    auto v = AnyHelper::extract_seq(result);

    return {
        static_cast<int>(AnyHelper::extract_long(v[0])),
        AnyHelper::to_json(v[1]),
        AnyHelper::to_json(v[2]),
        AnyHelper::to_json(v[3]),
        AnyHelper::extract_bool(v[4]),
        AnyHelper::extract_bool(v[5]),
        static_cast<int>(AnyHelper::extract_long(v[6])),
        static_cast<int>(AnyHelper::extract_long(v[7]))
    };
}

VisionVariableResponse CorbaClient::getVisionVariables() {
    const std::wstring names =
        L"@INTERNAL_CAMERA_PARAM_RAW/0 @INTERNAL_CAMERA_PARAM_RAW/1 "
        L"@INTERNAL_CAMERA_PARAM_RAW/2 @INTERNAL_CAMERA_PARAM_RAW/3 "
        L"@INTERNAL_CAMERA_PARAM/0 @INTERNAL_CAMERA_PARAM/1 "
        L"@INTERNAL_CAMERA_PARAM/2 @INTERNAL_CAMERA_PARAM/3 "
        L"@EXTERNAL_CAMERA_PARAM/0 @EXTERNAL_CAMERA_PARAM/1 "
        L"@EXTERNAL_CAMERA_PARAM/2 @EXTERNAL_CAMERA_PARAM/3";

    NxApi::NxObject_var vision = controller_->GetNxObject(L"Vision", L"");
    NxApi::NxObject_var vars   = vision->GetVariable(names.c_str(), L"");
    CORBA::Any result = vars->Execute(L"GetValues", AnyHelper::make_null());
    auto v = AnyHelper::extract_seq(result);

    return {
        AnyHelper::to_json(v[0]),  AnyHelper::to_json(v[1]),
        AnyHelper::to_json(v[2]),  AnyHelper::to_json(v[3]),
        AnyHelper::to_json(v[4]),  AnyHelper::to_json(v[5]),
        AnyHelper::to_json(v[6]),  AnyHelper::to_json(v[7]),
        AnyHelper::to_json(v[8]),  AnyHelper::to_json(v[9]),
        AnyHelper::to_json(v[10]), AnyHelper::to_json(v[11])
    };
}

TaskVariableResponse CorbaClient::getTaskVariables() {
    const std::wstring names =
        L"@STATUS @PRODUCTION_VOLUME @PRODUCTION_RECORD @CYCLE_TIME "
        L"@FLOW_DESCRIPTION/1 @FLOW_DESCRIPTION/2 @FLOW_DESCRIPTION/3 "
        L"@ASYNC_ACTION/1 @ASYNC_ACTION/2 @ASYNC_ACTION/3 @FLOW_ERROR_INFO";

    NxApi::NxObject_var task = controller_->GetNxObject(L"Task", L"");
    NxApi::NxObject_var vars = task->GetVariable(names.c_str(), L"");
    CORBA::Any result = vars->Execute(L"GetValues", AnyHelper::make_null());
    auto v = AnyHelper::extract_seq(result);

    return {
        static_cast<int>(AnyHelper::extract_long(v[0])),
        static_cast<int>(AnyHelper::extract_long(v[1])),
        static_cast<int>(AnyHelper::extract_long(v[2])),
        static_cast<int>(AnyHelper::extract_long(v[3])),
        AnyHelper::to_json(v[4]),  AnyHelper::to_json(v[5]),  AnyHelper::to_json(v[6]),
        AnyHelper::to_json(v[7]),  AnyHelper::to_json(v[8]),  AnyHelper::to_json(v[9]),
        AnyHelper::to_json(v[10])
    };
}

// ---- Controller variable writes ----

SimpleResponse CorbaClient::setSpeed(int value) {
    var_execute(L"@SPEED", L"SetValues",
                AnyHelper::make_single_var("value", AnyHelper::from_long(value)));
    return {"Set Speed Success.", 0, true, nullptr};
}

SimpleResponse CorbaClient::setDout(const std::vector<int>& value) {
    var_execute(L"@DOUT", L"SetValues",
                AnyHelper::make_single_var("value", AnyHelper::from_long_seq(value)));
    return {"Set Dout Success.", 0, true, nullptr};
}

SimpleResponse CorbaClient::getDout() {
    CORBA::Any result = var_execute(L"@DOUT", L"GetValues", AnyHelper::make_null());
    auto v = AnyHelper::extract_seq(result);
    return {"Get Dout Success.", 0, true, AnyHelper::to_json(v[0])};
}

SimpleResponse CorbaClient::setCs(const SaveCSRequest& req) {
    std::wstring varName = L"@CS/" + std::to_wstring(req.csNumber);
    std::vector<double> offset{req.x, req.y, req.z, req.roll, req.pitch, req.yaw};
    var_execute(varName, L"SetValues",
                AnyHelper::make_single_var("csOffset", AnyHelper::from_double_seq(offset)));
    return {"Set CS successful.", 0, true, nullptr};
}

SimpleResponse CorbaClient::setCs2(const SaveCSRequestwithName& req) {
    std::wstring varName = L"@CS/" + std::to_wstring(req.csNumber);
    std::vector<double> offset{req.x, req.y, req.z, req.roll, req.pitch, req.yaw};
    std::vector<std::pair<std::string, CORBA::Any>> params = {
        {"name",     AnyHelper::from_string(req.name)},
        {"refCS",    AnyHelper::from_long(req.refCS)},
        {"csOffset", AnyHelper::from_double_seq(offset)}
    };
    // Wrap in the triple-nest pattern: outer list contains one item (the kv params list)
    CORBA::Any inner = AnyHelper::make_params(params);
    std::vector<CORBA::Any> mid{inner};
    // Build [[params]] using make_any_seq indirectly via make_single_var-like nesting
    // We reuse make_params for the outer wrapping: [[kv1,kv2,kv3]]
    NxApi::NxObject_var var = controller_->GetVariable(varName.c_str(), L"");
    var->Execute(L"SetValues", inner);
    return {"Set CS successful.", 0, true, nullptr};
}

SimpleResponse CorbaClient::setCollisionCheck(bool value) {
    var_execute(L"@COLLISION_CHECK", L"SetValues",
                AnyHelper::make_single_var("value", AnyHelper::from_bool(value)));
    return {"Switch Collision Check successful.", 0, true, nullptr};
}

SaveCSResponse CorbaClient::getCs(const std::string& csNumber) {
    std::wstring varName = L"@CS/" + ws(csNumber);
    CORBA::Any result = var_execute(varName, L"GetValues", AnyHelper::make_null());
    auto outer = AnyHelper::extract_seq(result);
    auto inner = AnyHelper::extract_seq(outer[0]);

    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
    std::wstring csNum = AnyHelper::extract_wstring(inner[0]);
    CORBA::Long  refCs = AnyHelper::extract_long(inner[1]);
    auto offset = AnyHelper::extract_seq(inner[2]);

    SaveCSResponse r;
    r.csNumber = conv.to_bytes(csNum);
    r.refCs    = static_cast<int>(refCs);
    r.x     = AnyHelper::to_json(offset[0]).get<double>();
    r.y     = AnyHelper::to_json(offset[1]).get<double>();
    r.z     = AnyHelper::to_json(offset[2]).get<double>();
    r.roll  = AnyHelper::to_json(offset[3]).get<double>();
    r.pitch = AnyHelper::to_json(offset[4]).get<double>();
    r.yaw   = AnyHelper::to_json(offset[5]).get<double>();
    return r;
}

std::vector<SaveCSResponse> CorbaClient::getListofCs(const std::vector<std::string>& csList) {
    std::vector<SaveCSResponse> results;
    results.reserve(csList.size());
    for (const auto& cs : csList) {
        results.push_back(getCs(cs));
    }
    return results;
}

// ---- Task variable writes ----

SimpleResponse CorbaClient::setProductionVolume(int value) {
    subobj_var_execute(L"Task", L"", L"@PRODUCTION_VOLUME", L"SetValues",
                       AnyHelper::make_single_var("value", AnyHelper::from_long(value)));
    return {"Set Production Volume successful.", 0, true, nullptr};
}

SimpleResponse CorbaClient::setGlobalVars(const std::string& variableName, const std::string& value) {
    std::wstring varName = L"@GLOBAL_VARS/" + ws(variableName);
    subobj_var_execute(L"Task", L"", varName, L"SetValues",
                       AnyHelper::make_single_var("value", AnyHelper::from_string(value)));
    return {"Set Global Variables successful.", 0, true, nullptr};
}

SimpleResponse CorbaClient::getGlobalVars(const std::string& varName) {
    std::wstring wvar = L"@GLOBAL_VARS/" + ws(varName);
    CORBA::Any result = subobj_var_execute(L"Task", L"", wvar, L"GetValues", AnyHelper::make_null());
    auto v = AnyHelper::extract_seq(result);
    return {"Global Variable data: ", 0, true, AnyHelper::to_json(v[0])};
}

// ---- Controller extended commands ----

SimpleResponse CorbaClient::runCommand(const RunRequest& req) {
    std::vector<std::pair<std::string, CORBA::Any>> params = {
        {"command",   AnyHelper::from_string(req.command_type)},
        {"labelName", AnyHelper::from_string(req.labelName)},
        {"kind",      AnyHelper::from_long(req.interpolation)},
        {"arg",       AnyHelper::from_string(req.arguments)},
        {"speed",     AnyHelper::from_double(req.speed)},
        {"next",      AnyHelper::from_bool(req.next)}
    };
    CORBA::Any result = ctrl_execute(L"RunCommand", AnyHelper::make_params(params));
    CORBA::Long status = AnyHelper::extract_long(result);
    return simple_status(status, "Run command Successful.", "Run command Failed.");
}

// ---- Robot motion ----

DriveResponse CorbaClient::drive(const std::string& part, const DriveRequest& req) {
    std::vector<std::pair<std::string, CORBA::Any>> params = {
        {"no",    AnyHelper::from_long_seq(req.JointAxisNumbers)},
        {"mov",   AnyHelper::from_double_seq(req.JointAngles)},
        {"speed", AnyHelper::from_double(req.Speed)},
        {"next",  AnyHelper::from_bool(req.Next)}
    };
    CORBA::Any result = obj_execute(L"Robot", ws(part), L"Drive",
                                    AnyHelper::make_params(params));
    auto v = AnyHelper::extract_seq(result);
    return {static_cast<int>(AnyHelper::extract_long(v[0])),
            AnyHelper::to_json(v[1]).get<double>(),
            static_cast<int>(AnyHelper::extract_long(v[2]))};
}

DriveResponse CorbaClient::driveA(const std::string& part, const DriveRequest& req) {
    std::vector<std::pair<std::string, CORBA::Any>> params = {
        {"no",    AnyHelper::from_long_seq(req.JointAxisNumbers)},
        {"mov",   AnyHelper::from_double_seq(req.JointAngles)},
        {"speed", AnyHelper::from_double(req.Speed)},
        {"next",  AnyHelper::from_bool(req.Next)}
    };
    CORBA::Any result = obj_execute(L"Robot", ws(part), L"DriveA",
                                    AnyHelper::make_params(params));
    auto v = AnyHelper::extract_seq(result);
    return {static_cast<int>(AnyHelper::extract_long(v[0])),
            AnyHelper::to_json(v[1]).get<double>(),
            static_cast<int>(AnyHelper::extract_long(v[2]))};
}

DriveResponse CorbaClient::moveArm(const std::string& part, const MoveRequest& req) {
    std::vector<std::pair<std::string, CORBA::Any>> params = {
        {"comp",     AnyHelper::from_long(req.motionInterpolation)},
        {"csNo",     AnyHelper::from_long(req.csNumber)},
        {"speed",    AnyHelper::from_double(req.speed)},
        {"next",     AnyHelper::from_bool(req.next)},
        {"csOffset", AnyHelper::from_double_seq(req.csOffset)}
    };
    CORBA::Any result = obj_execute(L"Robot", ws(part), L"Move",
                                    AnyHelper::make_params(params));
    auto v = AnyHelper::extract_seq(result);
    return {static_cast<int>(AnyHelper::extract_long(v[0])),
            AnyHelper::to_json(v[1]).get<double>(),
            static_cast<int>(AnyHelper::extract_long(v[2]))};
}

SimpleResponse CorbaClient::waitMotion(const std::string& part, int queueSize) {
    std::vector<std::pair<std::string, CORBA::Any>> params = {
        {"queueSize", AnyHelper::from_long(queueSize)}
    };
    CORBA::Any result = obj_execute(L"Robot", ws(part), L"WaitMotion",
                                    AnyHelper::make_params(params));
    return {"WaitMotion command.", 0, static_cast<bool>(AnyHelper::extract_long(result) == 0), nullptr};
}

SimpleResponse CorbaClient::halt() {
    CORBA::Any result = obj_execute(L"Robot", L"All", L"Halt", AnyHelper::make_null());
    return simple_status(AnyHelper::extract_long(result), "Halt command successful.", "Halt command failed.");
}

SimpleResponse CorbaClient::hold() {
    CORBA::Any result = obj_execute(L"Robot", L"All", L"Hold", AnyHelper::make_null());
    return simple_status(AnyHelper::extract_long(result), "Hold command successful.", "Hold command failed.");
}

SimpleResponse CorbaClient::unHold() {
    CORBA::Any result = obj_execute(L"Robot", L"All", L"Unhold", AnyHelper::make_null());
    return simple_status(AnyHelper::extract_long(result), "UnHold command successful.", "UnHold command failed.");
}

// ---- Vision ----

GrabbedImageData CorbaClient::grabImage(const GrabImageRequest& req) {
    NxApi::NxObject_var vision = controller_->GetNxObject(L"Vision", L"");

    std::vector<std::pair<std::string, CORBA::Any>> grab_params = {
        {"cameraNo",          AnyHelper::from_long(req.cameraNumber)},
        {"exposure",          AnyHelper::from_double(req.exposure)},
        {"view",              AnyHelper::from_bool(req.view)},
        {"returnImage",       AnyHelper::from_bool(req.returnImage)},
        {"distortionCorrect", AnyHelper::from_bool(req.distortionCorrect)},
        {"format",            AnyHelper::from_long(req.format)}
    };
    vision->Execute(L"GrabImage", AnyHelper::make_params(grab_params));

    std::vector<std::pair<std::string, CORBA::Any>> get_params = {
        {"cameraNo",          AnyHelper::from_long(req.cameraNumber)},
        {"distortionCorrect", AnyHelper::from_bool(req.distortionCorrect)},
        {"format",            AnyHelper::from_long(req.format)}
    };
    CORBA::Any result = vision->Execute(L"GetGrabbedImage", AnyHelper::make_params(get_params));
    auto v = AnyHelper::extract_seq(result);

    // v[0]=status, v[1]=image bytes, v[2]=imageSize, v[3]=cameraAttributes, v[4]=imagingTime
    CORBA::Long status = AnyHelper::extract_long(v[0]);
    long long   imgTime = AnyHelper::to_json(v[4]).get<long long>();

    std::filesystem::create_directories("grabbed_images");
    std::string ext  = (req.format == 0) ? "bmp" : "jpg";
    std::string path = "grabbed_images/image_" + std::to_string(req.cameraNumber)
                       + "_" + std::to_string(imgTime) + "." + ext;

    // Extract byte sequence from v[1]
    CORBA::Any img_any = v[1];
    CORBA::TypeCode_var tc = img_any.type();
    if (tc->kind() == CORBA::tk_sequence) {
        auto bytes = AnyHelper::extract_seq(img_any);
        std::ofstream file(path, std::ios::binary);
        for (const auto& b : bytes) {
            CORBA::Octet byte_val = 0;
            CORBA::Any::to_octet to(byte_val);
            b >>= to;
            file.write(reinterpret_cast<const char*>(&byte_val), 1);
        }
    }

    return {
        static_cast<int>(status),
        AnyHelper::to_json(v[2]),
        AnyHelper::to_json(v[3]),
        imgTime,
        path
    };
}

// ---- Task commands ----

SimpleResponse CorbaClient::start() {
    std::vector<std::pair<std::string, CORBA::Any>> params = {{"mode", AnyHelper::from_long(2)}};
    CORBA::Any result = obj_execute(L"Task", L"", L"Start", AnyHelper::make_params(params));
    return simple_status(AnyHelper::extract_long(result), "Start command successful.", "Start command failed.");
}

SimpleResponse CorbaClient::stop() {
    std::vector<std::pair<std::string, CORBA::Any>> params = {{"mode", AnyHelper::from_long(3)}};
    CORBA::Any result = obj_execute(L"Task", L"", L"Stop", AnyHelper::make_params(params));
    return simple_status(AnyHelper::extract_long(result), "Stop command successful.", "Stop command failed.");
}

SimpleResponse CorbaClient::resume(const std::string& mode) {
    CORBA::Any result = obj_execute(L"Task", L"", L"Resume", AnyHelper::from_string(mode));
    return simple_status(AnyHelper::extract_long(result), "Resume command successful.", "Resume command failed.");
}

SimpleResponse CorbaClient::getTaskNames() {
    CORBA::Any result = obj_execute(L"Task", L"", L"GetTaskNames", AnyHelper::make_null());
    return {"Get Task Names successful.", 0, true, AnyHelper::to_json(result)};
}

SimpleResponse CorbaClient::setTask(const std::string& taskName) {
    std::vector<std::pair<std::string, CORBA::Any>> params = {
        {"taskName", AnyHelper::from_string(taskName)}
    };
    CORBA::Any result = obj_execute(L"Task", L"", L"SetTask", AnyHelper::make_params(params));
    return simple_status(AnyHelper::extract_long(result), "Set task command successful.", "Set task command failed.");
}

SimpleResponse CorbaClient::resetProdRecord() {
    CORBA::Any result = obj_execute(L"Task", L"", L"ResetProductionRecord", AnyHelper::make_null());
    return simple_status(AnyHelper::extract_long(result),
                         "Reset production record command successful.",
                         "Reset production record command failed.");
}
