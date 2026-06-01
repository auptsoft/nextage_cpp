#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

// ---- Simple responses ----

struct ConnectionStatusResponse {
    bool isSuccessful{false};
    std::string unconnectedPlugin;
    std::string camInitError;
    std::string headCamOffset;
    std::string camConnectionStatus;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ConnectionStatusResponse,
    isSuccessful, unconnectedPlugin, camInitError, headCamOffset, camConnectionStatus)

struct SimpleResponse {
    std::string message;
    int responseCode{0};
    bool isSuccessful{false};
    nlohmann::json data;  // str | int | list | null
};
inline void to_json(nlohmann::json& j, const SimpleResponse& r) {
    j = {{"message", r.message}, {"responseCode", r.responseCode},
         {"isSuccessful", r.isSuccessful}, {"data", r.data}};
}

// ---- Variable responses ----

struct ControllerVarResponse {
    std::string id;
    nlohmann::json controllerNames;
    bool autoMode{false};
    bool extControl{false};
    std::string apiControl;
    int speed{0};
    nlohmann::json dIn;
    nlohmann::json dOut;
    nlohmann::json CS0, CS1, CS2, CS3, CS4, CS5, CS6, CS7, CS8, CS9;
    bool emergencyStop{false};
    int emergencyStopCode{0};
    bool holdRestart{false};
    bool collisionCheck{false};
    int totalOperatingTime{0};
    nlohmann::json connectStatus;
    nlohmann::json pluginList;
    std::string IFVersion;
    std::string systemVersion;
};
inline void to_json(nlohmann::json& j, const ControllerVarResponse& r) {
    j = {
        {"id", r.id}, {"controllerNames", r.controllerNames},
        {"autoMode", r.autoMode}, {"extControl", r.extControl},
        {"apiControl", r.apiControl}, {"speed", r.speed},
        {"dIn", r.dIn}, {"dOut", r.dOut},
        {"CS0", r.CS0}, {"CS1", r.CS1}, {"CS2", r.CS2}, {"CS3", r.CS3},
        {"CS4", r.CS4}, {"CS5", r.CS5}, {"CS6", r.CS6}, {"CS7", r.CS7},
        {"CS8", r.CS8}, {"CS9", r.CS9},
        {"emergencyStop", r.emergencyStop}, {"emergencyStopCode", r.emergencyStopCode},
        {"holdRestart", r.holdRestart}, {"collisionCheck", r.collisionCheck},
        {"totalOperatingTime", r.totalOperatingTime},
        {"connectStatus", r.connectStatus}, {"pluginList", r.pluginList},
        {"IFVersion", r.IFVersion}, {"systemVersion", r.systemVersion}
    };
}

struct RobotVariableResponse {
    int servoOn{0};
    nlohmann::json servoStatus;
    nlohmann::json jointAngle;
    nlohmann::json actualJointAngle;
    bool busyStatus{false};
    bool pause{false};
    int pauseCode{0};
    int totalOperatingTime{0};
};
inline void to_json(nlohmann::json& j, const RobotVariableResponse& r) {
    j = {
        {"servoOn", r.servoOn}, {"servoStatus", r.servoStatus},
        {"jointAngle", r.jointAngle}, {"actualJointAngle", r.actualJointAngle},
        {"busyStatus", r.busyStatus}, {"pause", r.pause},
        {"pauseCode", r.pauseCode}, {"totalOperatingTime", r.totalOperatingTime}
    };
}

struct VisionVariableResponse {
    nlohmann::json internalCameraParamRaw0, internalCameraParamRaw1,
                   internalCameraParamRaw2, internalCameraParamRaw3;
    nlohmann::json internalCameraParam0, internalCameraParam1,
                   internalCameraParam2, internalCameraParam3;
    nlohmann::json externalCameraParam0, externalCameraParam1,
                   externalCameraParam2, externalCameraParam3;
};
inline void to_json(nlohmann::json& j, const VisionVariableResponse& r) {
    j = {
        {"internalCameraParamRaw0", r.internalCameraParamRaw0},
        {"internalCameraParamRaw1", r.internalCameraParamRaw1},
        {"internalCameraParamRaw2", r.internalCameraParamRaw2},
        {"internalCameraParamRaw3", r.internalCameraParamRaw3},
        {"internalCameraParam0", r.internalCameraParam0},
        {"internalCameraParam1", r.internalCameraParam1},
        {"internalCameraParam2", r.internalCameraParam2},
        {"internalCameraParam3", r.internalCameraParam3},
        {"externalCameraParam0", r.externalCameraParam0},
        {"externalCameraParam1", r.externalCameraParam1},
        {"externalCameraParam2", r.externalCameraParam2},
        {"externalCameraParam3", r.externalCameraParam3}
    };
}

struct TaskVariableResponse {
    int status{0};
    int productionVolume{0};
    int productionRecord{0};
    int cycleTime{0};
    nlohmann::json flowDescription1, flowDescription2, flowDescription3;
    nlohmann::json asyncAction1, asyncAction2, asyncAction3;
    nlohmann::json flowErrorInfo;
};
inline void to_json(nlohmann::json& j, const TaskVariableResponse& r) {
    j = {
        {"status", r.status}, {"productionVolume", r.productionVolume},
        {"productionRecord", r.productionRecord}, {"cycleTime", r.cycleTime},
        {"flowDescription1", r.flowDescription1},
        {"flowDescription2", r.flowDescription2},
        {"flowDescription3", r.flowDescription3},
        {"asyncAction1", r.asyncAction1}, {"asyncAction2", r.asyncAction2},
        {"asyncAction3", r.asyncAction3}, {"flowErrorInfo", r.flowErrorInfo}
    };
}

struct SaveCSResponse {
    std::string csNumber;
    int refCs{0};
    double x{0}, y{0}, z{0};
    double roll{0}, pitch{0}, yaw{0};
};
inline void to_json(nlohmann::json& j, const SaveCSResponse& r) {
    j = {{"csNumber", r.csNumber}, {"refCs", r.refCs},
         {"x", r.x}, {"y", r.y}, {"z", r.z},
         {"roll", r.roll}, {"pitch", r.pitch}, {"yaw", r.yaw}};
}

struct DriveResponse {
    int status{0};
    double executionTime{0};
    int info{0};
};
inline void to_json(nlohmann::json& j, const DriveResponse& r) {
    j = {{"status", r.status}, {"executionTime", r.executionTime}, {"info", r.info}};
}

struct GrabbedImageData {
    int status{0};
    nlohmann::json imageSize;
    nlohmann::json cameraAttributes;
    long long imagingTime{0};
    std::string imageFilePath;
};
inline void to_json(nlohmann::json& j, const GrabbedImageData& r) {
    j = {{"status", r.status}, {"imageSize", r.imageSize},
         {"cameraAttributes", r.cameraAttributes},
         {"imagingTime", r.imagingTime}, {"imageFilePath", r.imageFilePath}};
}
