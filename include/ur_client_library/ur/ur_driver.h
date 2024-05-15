// this is for emacs file handling -*- mode: c++; indent-tabs-mode: nil -*-

// -- BEGIN LICENSE BLOCK ----------------------------------------------
// Copyright 2019 FZI Forschungszentrum Informatik
// Created on behalf of Universal Robots A/S
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// -- END LICENSE BLOCK ------------------------------------------------

//----------------------------------------------------------------------
/*!\file
 *
 * \author  Felix Exner exner@fzi.de
 * \date    2019-04-11
 *
 */
//----------------------------------------------------------------------
#ifndef UR_CLIENT_LIBRARY_UR_UR_DRIVER_H_INCLUDED
#define UR_CLIENT_LIBRARY_UR_UR_DRIVER_H_INCLUDED

#include <functional>
#include <chrono>
#include <thread>

#include "ur_client_library/rtde/rtde_client.h"
#include "ur_client_library/control/reverse_interface.h"
#include "ur_client_library/control/trajectory_point_interface.h"
#include "ur_client_library/control/script_command_interface.h"
#include "ur_client_library/control/script_sender.h"
#include "ur_client_library/ur/tool_communication.h"
#include "ur_client_library/ur/version_information.h"
#include "ur_client_library/ur/robot_receive_timeout.h"
#include "ur_client_library/primary/robot_message/version_message.h"
#include "ur_client_library/rtde/rtde_writer.h"

namespace urcl
{
/*!
 * \brief This is the main class for interfacing the driver.
 *
 * It sets up all the necessary socket connections and handles the data exchange with the robot.
 * Use this classes methods to access and write data.
 *
 */
class UrDriver
{
public:
  /*!
   * \brief Constructs a new UrDriver object.
   * Upon initialization this class will check the calibration checksum reported from the robot and
   * compare it to a checksum given by the user. If the checksums don't match, the driver will output
   * an error message. This is critical if you want to do forward or inverse kinematics based on the
   * model that the given calibration checksum matches to.
   *
   * An RTDE connection to the robot will be established using the given recipe files. However, RTDE
   * communication will not be started automatically, as this requires an external structure to read
   * data from the RTDE client using the getDataPackage() method periodically. Once this is setup,
   * please use the startRTDECommunication() method to actually start RTDE communication.
   *
   * Furthermore, initialization creates a ScriptSender member object that will read a URScript file
   * from \p script_file, perform a number of replacements to populate the script with dynamic data.
   * See the implementation for details.
   *
   * \param robot_ip IP-address under which the robot is reachable.
   * \param script_file URScript file that should be sent to the robot.
   * \param output_recipe_file Filename where the output recipe is stored in.
   * \param input_recipe_file Filename where the input recipe is stored in.
   * \param handle_program_state Function handle to a callback on program state changes. For this to
   * work, the URScript program will have to send keepalive signals to the \p reverse_port. I no
   * keepalive signal can be read, program state will be false.
   * \param headless_mode Parameter to control if the driver should be started in headless mode.
   * \param tool_comm_setup Configuration for using the tool communication.
   * calibration reported by the robot.
   * \param reverse_port Port that will be opened by the driver to allow direct communication between the driver
   * and the robot controller.
   * \param script_sender_port The driver will offer an interface to receive the program's URScript on this port. If
   * the robot cannot connect to this port, `External Control` will stop immediately.
   * \param non_blocking_read Enable non-blocking mode for read (useful when used with combined_robot_hw)
   * \param servoj_gain Proportional gain for arm joints following target position, range [100,2000]
   * \param servoj_lookahead_time Time [S], range [0.03,0.2] smoothens the trajectory with this lookahead time
   * \param reverse_ip IP address that the reverse_port will get bound to. If not specified, the IP
   * address of the interface that is used for connecting to the robot's RTDE port will be used.
   * \param trajectory_port Port used for sending trajectory points to the robot in case of
   * trajectory forwarding.
   * \param script_command_port Port used for forwarding script commands to the robot. The script commands will be
   * executed locally on the robot.
   * \param force_mode_damping The damping parameter used when the robot is in force mode, range [0,1]
   * \param force_mode_gain_scaling Scales the gain used when the robot is in force mode, range [0,2] (only e-series)
   */
  UrDriver(const std::string& robot_ip, const std::string& script_file, const std::string& output_recipe_file,
           const std::string& input_recipe_file, std::function<void(bool)> handle_program_state, bool headless_mode,
           std::unique_ptr<ToolCommSetup> tool_comm_setup, const uint32_t reverse_port = 50001,
           const uint32_t script_sender_port = 50002, int servoj_gain = 2000, double servoj_lookahead_time = 0.03,
           bool non_blocking_read = false, const std::string& reverse_ip = "", const uint32_t trajectory_port = 50003,
           const uint32_t script_command_port = 50004, double force_mode_damping = 0.025,
           double force_mode_gain_scaling = 0.5,
           double deafult_payload_mass = 0.0, double default_payload_cog_x = 0.0, double default_payload_cog_y = 0.0, double default_payload_cog_z = 0.0);

  /*!
   * \brief Constructs a new UrDriver object.
   * \param robot_ip IP-address under which the robot is reachable.
   * \param script_file URScript file that should be sent to the robot.
   * \param output_recipe_file Filename where the output recipe is stored in.
   * \param input_recipe_file Filename where the input recipe is stored in.
   * \param handle_program_state Function handle to a callback on program state changes. For this to
   * work, the URScript program will have to send keepalive signals to the \p reverse_port. I no
   * keepalive signal can be read, program state will be false.
   * \param headless_mode Parameter to control if the driver should be started in headless mode.
   * \param tool_comm_setup Configuration for using the tool communication.
   * \param calibration_checksum Expected checksum of calibration. Will be matched against the
   * calibration reported by the robot.
   * \param reverse_port Port that will be opened by the driver to allow direct communication between the driver
   * and the robot controller.
   * \param script_sender_port The driver will offer an interface to receive the program's URScript on this port. If
   * the robot cannot connect to this port, `External Control` will stop immediately.
   * \param non_blocking_read Enable non-blocking mode for read (useful when used with combined_robot_hw)
   * \param servoj_gain Proportional gain for arm joints following target position, range [100,2000]
   * \param servoj_lookahead_time Time [S], range [0.03,0.2] smoothens the trajectory with this lookahead time
   * \param reverse_ip IP address that the reverse_port will get bound to. If not specified, the IP
   * address of the interface that is used for connecting to the robot's RTDE port will be used.
   * \param trajectory_port Port used for sending trajectory points to the robot in case of
   * trajectory forwarding.
   * \param script_command_port Port used for forwarding script commands to the robot. The script commands will be
   * executed locally on the robot.
   * \param force_mode_damping The damping parameter used when the robot is in force mode, range [0,1]
   * \param force_mode_gain_scaling Scales the gain used when the robot is in force mode, range [0,2] (only e-series)
   */
  UrDriver(const std::string& robot_ip, const std::string& script_file, const std::string& output_recipe_file,
           const std::string& input_recipe_file, std::function<void(bool)> handle_program_state, bool headless_mode,
           std::unique_ptr<ToolCommSetup> tool_comm_setup, const std::string& calibration_checksum = "",
           const uint32_t reverse_port = 50001, const uint32_t script_sender_port = 50002, int servoj_gain = 2000,
           double servoj_lookahead_time = 0.03, bool non_blocking_read = false, const std::string& reverse_ip = "",
           const uint32_t trajectory_port = 50003, const uint32_t script_command_port = 50004,
           double force_mode_damping = 0.025, double force_mode_gain_scaling = 0.5,
           double deafult_payload_mass = 0.0, double default_payload_cog_x = 0.0, double default_payload_cog_y = 0.0, double default_payload_cog_z = 0.0);
  /*!
   * \brief Constructs a new UrDriver object.
   *
   * \param robot_ip IP-address under which the robot is reachable.
   * \param script_file URScript file that should be sent to the robot.
   * \param output_recipe_file Filename where the output recipe is stored in.
   * \param input_recipe_file Filename where the input recipe is stored in.
   * \param handle_program_state Function handle to a callback on program state changes. For this to
   * work, the URScript program will have to send keepalive signals to the \p reverse_port. I no
   * keepalive signal can be read, program state will be false.
   * \param headless_mode Parameter to control if the driver should be started in headless mode.
   * \param calibration_checksum Expected checksum of calibration. Will be matched against the
   * calibration reported by the robot.
   * \param reverse_port Port that will be opened by the driver to allow direct communication between the driver
   * and the robot controller
   * \param script_sender_port The driver will offer an interface to receive the program's URScript on this port.
   * If the robot cannot connect to this port, `External Control` will stop immediately.
   * \param non_blocking_read Enable non-blocking mode for read (useful when used with combined_robot_hw)
   * \param servoj_gain Proportional gain for arm joints following target position, range [100,2000]
   * \param servoj_lookahead_time Time [S], range [0.03,0.2] smoothens the trajectory with this lookahead time
   * \param reverse_ip IP address that the reverse_port will get bound to. If not specified, the IP
   * address of the interface that is used for connecting to the robot's RTDE port will be used.
   * \param trajectory_port Port used for sending trajectory points to the robot in case of
   * trajectory forwarding.
   * \param script_command_port Port used for forwarding script commands to the robot. The script commands will be
   * executed locally on the robot.
   * \param force_mode_damping The damping parameter used when the robot is in force mode, range [0,1]
   * \param force_mode_gain_scaling Scales the gain used when the robot is in force mode, range [0,2] (only e-series)
   */
  UrDriver(const std::string& robot_ip, const std::string& script_file, const std::string& output_recipe_file,
           const std::string& input_recipe_file, std::function<void(bool)> handle_program_state, bool headless_mode,
           const std::string& calibration_checksum = "", const uint32_t reverse_port = 50001,
           const uint32_t script_sender_port = 50002, int servoj_gain = 2000, double servoj_lookahead_time = 0.03,
           bool non_blocking_read = false, const std::string& reverse_ip = "", const uint32_t trajectory_port = 50003,
           const uint32_t script_command_port = 50004, double force_mode_damping = 0.025,
           double force_mode_gain_scaling = 0.5)
    : UrDriver(robot_ip, script_file, output_recipe_file, input_recipe_file, handle_program_state, headless_mode,
               std::unique_ptr<ToolCommSetup>{}, calibration_checksum, reverse_port, script_sender_port, servoj_gain,
               servoj_lookahead_time, non_blocking_read, reverse_ip, trajectory_port, script_command_port,
               force_mode_damping, force_mode_gain_scaling)
  {
  }

  virtual ~UrDriver() = default;

  /*!
   * \brief Access function to receive the latest data package sent from the robot through RTDE
   * interface.
   *
   * \returns The latest data package on success, a nullptr if no package can be found inside a preconfigured time
   * window.
   */
  std::unique_ptr<rtde_interface::DataPackage> getDataPackage();

  uint32_t getControlFrequency() const
  {
    return rtde_frequency_;
  }

  /*!
   * \brief Writes a joint command together with a keepalive signal onto the socket being sent to
   * the robot.
   *
   * \param values Desired joint positions
   * \param control_mode Control mode this command is assigned to.
   * \param robot_receive_timeout The read timeout configuration for the reverse socket running in the external
   * control script on the robot. Use with caution when dealing with realtime commands as the robot
   * expects to get a new control signal each control cycle. Note the timeout cannot be higher than 1 second for
   * realtime commands.
   *
   * \returns True on successful write.
   */
  bool writeJointCommand(const vector6d_t& values, const comm::ControlMode control_mode,
                         const RobotReceiveTimeout& robot_receive_timeout = RobotReceiveTimeout::millisec(20));

  /*!
   * \brief Writes a trajectory point onto the dedicated socket.
   *
   * \param positions Desired joint or cartesian positions
   * \param cartesian True, if the point sent is cartesian, false if joint-based
   * \param goal_time Time for the robot to reach this point
   * \param blend_radius  The radius to be used for blending between control points
   *
   * \returns True on successful write.
   */
  bool writeTrajectoryPoint(const vector6d_t& positions, const bool cartesian, const float goal_time = 0.0,
                            const float blend_radius = 0.052);

  /*!
   * \brief Writes a trajectory spline point for quintic spline interpolation onto the dedicated socket.
   *
   * \param positions Desired joint positions
   * \param velocities Desired joint velocities
   * \param accelerations Desired joint accelerations
   * \param goal_time Time for the robot to reach this point
   *
   * \returns True on successful write.
   */
  bool writeTrajectorySplinePoint(const vector6d_t& positions, const vector6d_t& velocities,
                                  const vector6d_t& accelerations, const float goal_time = 0.0);

  /*!
   * \brief Writes a trajectory spline point for cubic spline interpolation onto the dedicated socket.
   *
   * \param positions Desired joint positions
   * \param velocities Desired joint velocities
   * \param goal_time Time for the robot to reach this point
   *
   * \returns True on successful write.
   */
  bool writeTrajectorySplinePoint(const vector6d_t& positions, const vector6d_t& velocities,
                                  const float goal_time = 0.0);

  /*!
   * \brief Writes a trajectory spline point for quadratic spline interpolation onto the dedicated socket.
   *
   * \param positions Desired joint positions
   * \param goal_time Time for the robot to reach this point
   *
   * \returns True on successful write.
   */
  bool writeTrajectorySplinePoint(const vector6d_t& positions, const float goal_time = 0.0);

  /*!
   * \brief Writes a control message in trajectory forward mode.
   *
   * \param trajectory_action The action to be taken, such as starting a new trajectory
   * \param point_number The number of points of a new trajectory to be sent
   * \param robot_receive_timeout The read timeout configuration for the reverse socket running in the external
   * control script on the robot. If you want to make the read function blocking then use RobotReceiveTimeout::off()
   * function to create the RobotReceiveTimeout object
   *
   * \returns True on successful write.
   */
  bool
  writeTrajectoryControlMessage(const control::TrajectoryControlMessage trajectory_action, const int point_number = 0,
                                const RobotReceiveTimeout& robot_receive_timeout = RobotReceiveTimeout::millisec(200));

  /*!
   * \brief Writes a control message in freedrive mode.
   *
   * \param freedrive_action The action to be taken, such as starting or stopping freedrive
   * \param robot_receive_timeout The read timeout configuration for the reverse socket running in the external
   * control script on the robot. If you want to make the read function blocking then use RobotReceiveTimeout::off()
   * function to create the RobotReceiveTimeout object
   *
   * \returns True on successful write.
   */
  bool
  writeFreedriveControlMessage(const control::FreedriveControlMessage freedrive_action,
                               const RobotReceiveTimeout& robot_receive_timeout = RobotReceiveTimeout::millisec(200));

  /*!
   * \brief Zero the force torque sensor (only availbe on e-Series). Note:  It requires the external control script to
   * be running or the robot to be in headless mode
   *
   * \returns True on successful write.
   */
  bool zeroFTSensor();

  /*!
   * \brief Set the payload mass and center of gravity. Note: It requires the external control script to be running or
   * the robot to be in headless mode.
   *
   * \param mass mass in kilograms
   * \param cog Center of Gravity, a vector [CoGx, CoGy, CoGz] specifying the displacement (in meters) from the
   * toolmount
   *
   * \returns True on successful write.
   */
  bool setPayload(const float mass, const vector3d_t& cog);

  /*!
   * \brief Set the tool voltage. Note: It requires the external control script to be running or the robot to be in
   * headless mode.
   *
   * \param voltage tool voltage.
   *
   * \returns True on successful write.
   */
  bool setToolVoltage(const ToolVoltage voltage);

  /*!
   * \brief Start the robot to be controlled in force mode.
   *
   * \param task_frame A pose vector that defines the force frame relative to the base frame
   * \param selection_vector A 6d vector of 0s and 1s. 1 means that the robot will be compliant in the corresponding
   * axis of the task frame
   * \param wrench 6d vector of forces/torques [x,y,z,rotX,rotY,rotZ] that the robot will apply to its environment. The
   * robot adjusts its position along/about compliant axis in order to achieve the specified force/torque. Values have
   * no effect for non-compliant axes.
   * \param type An integer [1;3] specifying how the robot interprets the force frame.
   *  1: The force frame is transformed in a way such that its y-axis is aligned with a vector pointing from the robot
   *  tcp towards the origin of the force frame
   *  2: The force frame is not transformed
   *  3: The force frame is transformed in a way such that its x-axis is the projection of the robot tcp velocity vector
   *  onto the x-y plane of the force frame
   * \param limits (double) 6d vector. For compliant axes, these values are the maximum allowed tcp speed
   * along/about the axis. For non-compliant axes, these values are the maximum allowed deviation along/about an axis
   * between the actual tcp position and the one set by the program
   *
   * \returns True, if the write was performed successfully, false otherwise.
   */
  bool startForceMode(const vector6d_t& task_frame, const vector6uint32_t& selection_vector, const vector6d_t& wrench,
                      const unsigned int type, const vector6d_t& limits);

  /*!
   * \brief Stop force mode and put the robot into normal operation mode.
   *
   * \returns True, if the write was performed successfully, false otherwise.
   */
  bool endForceMode();

  /*!
   * \brief This will make the robot look for tool contact in the tcp directions that the robot is currently
   * moving. Once a tool contact has been detected all movements will be canceled. Call endToolContact to enable
   * movements again.
   *
   * \returns True, if the write was performed successfully, false otherwise.
   */
  bool startToolContact();

  /*!
   * \brief This will stop the robot from looking for a tool contact, it will also enable sending move commands to the
   * robot again if the robot's tool is in contact
   *
   * \returns True, if the write was performed successfully, false otherwise.
   */
  bool endToolContact();

  /*!
   * \brief Set the tool contact result callback object
   *
   * \param callback Callback function that will be triggered when the robot enters tool contact
   */
  void setToolContactResultCallback(std::function<void(control::ToolContactResult)> callback);

  /*!
   * \brief Set the direction of the acceleration experienced by the robot. When the robot mounting is fixed, this
   * corresponds to an accleration of g away from the earth’s centre
   *
   * \param gravity  A vector [gravity_x, gravity_y, gravity_z] 3D vector, describing the direction of the gravity,
   * relative to the base of the robot
   *
   * \returns True, if the write was performed successfully, false otherwise.
   */
  bool setGravity(const vector3d_t& gravity);

  /*!
   * \brief Activate dynamic payload setting loop
   *
   * \param command_type  Specifies if the command is for front lift, front drag or top lift
   *
   * \param move_distance  Distance to move for payload estimation in meters
   * 
   * \returns True, if the write was performed successfully, false otherwise.
   */
  bool startPayloadEstimation(control::PayloadEstimType command_type, double move_distance, double secondary_move_distance, double move_speed);

  /*!
   * \brief Set the payload estimation result callback
   *
   * \param callback Callback function that will be triggered when the robot completes the payload estimation
   */
  void setPayloadEstimationResultCallback(std::function<void()> callback);

  /*!
   * \brief Write a keepalive signal only.
   *
   * This signals the robot that the connection is still
   * active in times when no commands are to be sent (e.g. no controller is active.)
   *
   * \param robot_receive_timeout The read timeout configuration for the reverse socket running in the external
   * control script on the robot. If you want to make the read function blocking then use RobotReceiveTimeout::off()
   * function to create the RobotReceiveTimeout object
   *
   * \returns True on successful write.
   */
  bool writeKeepalive(const RobotReceiveTimeout& robot_receive_timeout = RobotReceiveTimeout::millisec(1000));

  /*!
   * \brief Starts the RTDE communication.
   *
   * After initialization, the cyclic RTDE communication is not started automatically, so that data
   * consumers can be started also at a later point.
   */
  void startRTDECommunication();

  /*!
   * \brief Sends a stop command to the socket interface which will signal the program running on
   * the robot to no longer listen for commands sent from the remote pc.
   *
   * \returns True on successful write.
   */
  bool stopControl();

  /*!
   * \brief Checks if the kinematics information in the used model fits the actual robot.
   *
   * \param checksum Hash of the used kinematics information
   *
   * \returns True if the robot's calibration checksum matches the one given to the checker. False
   * if it doesn't match or the check was not yet performed.
   */
  bool checkCalibration(const std::string& checksum);

  /*!
   * \brief Getter for the RTDE writer used to write to the robot's RTDE interface.
   *
   * \returns The active RTDE writer
   */
  rtde_interface::RTDEWriter& getRTDEWriter();

  /*!
   * \brief Sends a custom script program to the robot.
   *
   * The given code must be valid according the UR Scripting Manual.
   *
   * \param program URScript code that shall be executed by the robot.
   *
   * \returns true on successful upload, false otherwise.
   */
  bool sendScript(const std::string& program);

  /*!
   * \brief Sends the external control program to the robot.
   *
   * Only for use in headless mode, as it replaces the use of the URCaps program.
   *
   * \returns true on successful upload, false otherwise
   */
  bool sendRobotProgram();

  /*!
   * \brief Returns version information about the currently connected robot
   */
  const VersionInformation& getVersion()
  {
    return robot_version_;
  }

  /*!
   * \brief Getter for the RTDE output recipe used in the RTDE client.
   *
   * \returns The used RTDE output recipe
   */
  std::vector<std::string> getRTDEOutputRecipe();

  /*!
   * \brief Set the Keepalive count. This will set the number of allowed timeout reads on the robot.
   *
   * \param count Number of allowed timeout reads on the robot.
   */
  [[deprecated("Set keepaliveCount is deprecated, instead set the robot receive timeout directly in the write "
               "commands.")]] void
  setKeepaliveCount(const uint32_t count);

  /*!
   * \brief Register a callback for the robot-based trajectory execution completion.
   *
   * One mode of robot control is to forward a complete trajectory to the robot for execution.
   * When the execution is done, the callback function registered here will be triggered.
   *
   * \param trajectory_done_cb Callback function that will be triggered in the event of finishing
   * a trajectory execution
   */
  void registerTrajectoryDoneCallback(std::function<void(control::TrajectoryResult)> trajectory_done_cb)
  {
    trajectory_interface_->setTrajectoryEndCallback(trajectory_done_cb);
  }

  /*!
   * \brief Register a callback for the robot-based tool contact execution completion.
   *
   * If a tool contact is detected or tool contact is canceled, this callback function will be triggered mode of robot
   * control is to move until tool contact. It requires that tool contact has been started using startToolContact()
   *
   * \param tool_contact_result_cb Callback function that will be triggered when the robot enters tool contact
   */
  void registerToolContactResultCallback(std::function<void(control::ToolContactResult)> tool_contact_result_cb)
  {
    script_command_interface_->setToolContactResultCallback(tool_contact_result_cb);
  }

private:
  static std::string readScriptFile(const std::string& filename);
  void tryReconnectScriptCommandInterface();

  int rtde_frequency_;
  comm::INotifier notifier_;
  std::unique_ptr<rtde_interface::RTDEClient> rtde_client_;
  std::unique_ptr<control::ReverseInterface> reverse_interface_;
  std::unique_ptr<control::TrajectoryPointInterface> trajectory_interface_;
  std::unique_ptr<control::ScriptCommandInterface> script_command_interface_;
  std::unique_ptr<control::ScriptSender> script_sender_;
  std::unique_ptr<comm::URStream<primary_interface::PrimaryPackage>> primary_stream_;
  std::unique_ptr<comm::URStream<primary_interface::PrimaryPackage>> secondary_stream_;

  uint32_t servoj_gain_;
  double servoj_lookahead_time_;
  std::chrono::milliseconds step_time_;

  std::function<void(bool)> handle_program_state_;

  std::string robot_ip_;
  bool in_headless_mode_;
  std::string full_robot_program_;

  int get_packet_timeout_;
  bool non_blocking_read_;

  VersionInformation robot_version_;
};
}  // namespace urcl
#endif  // ifndef UR_CLIENT_LIBRARY_UR_UR_DRIVER_H_INCLUDED
