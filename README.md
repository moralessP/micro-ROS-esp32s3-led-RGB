# micro-ROS-esp32s3-led-RGB

Control onboard led RGB esp32s3 with micros-ROS and ROS2 environment

## Prerequisites 

- Install [ROS2 Humble](https://docs.ros.org/en/humble/index.html)

## Install

```bash
sudo apt install python3-vcstool python3-colcon-common-extensions git wget
```
#### 1. Create a workspace folder

```bash
mkdir -p ~/uros_ws
```
#### 2. Install micro-ROS-AGENT

```bash
cd ~/uros_ws
git clone -b humble --recursive https://github.com/micro-ROS/micro-ROS-Agent.git
rosdep install --rosdistro $ROS_DISTRO --from-paths micro-ROS-AGENT -i -r -y
cd ~/uros_ws/micro-ROS-AGENT
colcon build --symlink-install --cmake-args -DCMAKE_BUILD_TYPE=Release
```
#### 3. Clone repository

```bash
cd ~/uros_ws
git clone --recursive https://github.com/moralessP/micro-ROS-esp32s3-led-RGB.git
cd ~/uros_ws/micros-ROS-esp32s3-led-RGB
```
Set IP address and communication port (8888). Also SSID and password 
```bash
idf.py set-target esp32s3
idf.py menuconfig
idf.py build   
```
#### 4. Run

Flash firmware
```bash
idf.py -p PORT flash
```
Open a terminal  
```bash
source ~/uros_ws/micro-ROS-AGENT/install/setup.bash
ros2 run micro_ros_agent micro_ros_agent udp4 --port 8888
```
#### 5. Usage 

Open a new terminal  
```bash
ros2 topic pub --once /led_rgb std_msgs/msg/UInt8MultiArray "{data: [255, 255, 255]}" # send R=255 G=255 B=255 to led_rgb topic 
```