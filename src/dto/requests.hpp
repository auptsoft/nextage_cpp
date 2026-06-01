#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

// ---- DrivePart enum ----

enum class DrivePart {
    NECK,
    WAIST,
    LARM,
    RARM,
    ALL
};

inline std::string drive_part_to_string(DrivePart p) {
    switch (p) {
        case DrivePart::NECK:  return "Neck";
        case DrivePart::WAIST: return "Chest";
        case DrivePart::LARM:  return "LArm";
        case DrivePart::RARM:  return "RArm";
        case DrivePart::ALL:   return "All";
    }
    return "All";
}

inline DrivePart drive_part_from_string(const std::string& s) {
    if (s == "Neck")  return DrivePart::NECK;
    if (s == "Chest") return DrivePart::WAIST;
    if (s == "LArm")  return DrivePart::LARM;
    if (s == "RArm")  return DrivePart::RARM;
    return DrivePart::ALL;
}

// ---- Request structs ----

struct ConnectRequest {
    std::string ip_address;
    int port{2809};
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ConnectRequest, ip_address, port)

struct SaveCSRequest {
    double x{0}, y{0}, z{0};
    double roll{0}, pitch{0}, yaw{0};
    int csNumber{0};
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SaveCSRequest, x, y, z, roll, pitch, yaw, csNumber)

struct SaveCSRequestwithName : SaveCSRequest {
    int refCS{0};
    std::string name;
};
inline void from_json(const nlohmann::json& j, SaveCSRequestwithName& r) {
    from_json(j, static_cast<SaveCSRequest&>(r));
    j.at("refCS").get_to(r.refCS);
    j.at("name").get_to(r.name);
}

struct DriveRequest {
    std::vector<int> JointAxisNumbers;
    std::vector<double> JointAngles;
    double Speed{0};
    bool Next{false};
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(DriveRequest, JointAxisNumbers, JointAngles, Speed, Next)

struct MoveRequest {
    int motionInterpolation{0};
    int csNumber{0};
    double speed{0};
    bool next{false};
    std::vector<double> csOffset;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MoveRequest, motionInterpolation, csNumber, speed, next, csOffset)

struct RunRequest {
    std::string command_type;
    std::string labelName;
    int interpolation{0};
    std::string arguments;
    double speed{0};
    bool next{false};
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RunRequest, command_type, labelName, interpolation, arguments, speed, next)

struct GrabImageRequest {
    int cameraNumber{0};
    bool view{false};
    bool returnImage{false};
    bool distortionCorrect{false};
    int format{1};   // 0=BMP, 1=JPG
    double exposure{0};
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(GrabImageRequest, cameraNumber, view, returnImage, distortionCorrect, format, exposure)

struct SetSpeedRequest {
    int value{0};
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SetSpeedRequest, value)

struct SetDoutRequest {
    std::vector<int> value;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SetDoutRequest, value)

struct SetCollisionCheckRequest {
    bool value{false};
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SetCollisionCheckRequest, value)

struct SetProductionVolumeRequest {
    int value{0};
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SetProductionVolumeRequest, value)

struct SetGlobalVarsRequest {
    std::string name;
    std::string value;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SetGlobalVarsRequest, name, value)

struct SetTaskRequest {
    std::string taskName;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SetTaskRequest, taskName)

struct ResumeRequest {
    std::string mode;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ResumeRequest, mode)

struct WaitMotionRequest {
    std::string part;
    int queue{0};
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(WaitMotionRequest, part, queue)
