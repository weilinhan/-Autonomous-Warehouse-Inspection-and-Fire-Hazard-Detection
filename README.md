# 🤖 Fire Detection and Warehouse Inspection Robot on Jetson Nano (Transbot-based)

This project presents the design and implementation of a **multimodal warehouse inspection robot** running on the **NVIDIA Jetson Nano** and the **Transbot mobile robot platform**.  
The system integrates **computer vision (YOLOv5)**, **temperature and gas sensing**, and **autonomous navigation** to provide a **reliable and efficient solution** for warehouse inspection with a focus on **early fire detection**.

---

## 🚀 Features

- 🔥 Fire and smoke detection with YOLOv5  
- 🌡️ Environmental monitoring via temperature and gas sensors  
- 🤖 Autonomous navigation with line following & obstacle avoidance  
- 🛰️ Localization with ArTags
- ☁️ **AWS IoT integration** for cloud-based data storage and remote monitoring   
- 💻 User interface with visualization & alarm system (PC application)  
- 📱 Notifications on mobile devices for critical alerts  
- ⚡ Real-time processing on Jetson Nano (edge computing)  

---

## 🏗️ System Architecture

- **Sensing:** Camera (AR0234 Global Shutter USB Camera,Deepth camera ORBBEC Astra), temperature sensor DHT11, gas sensor MQ135  
- **Processing:** Jetson Nano (YOLOv5 inference, navigation logic),ESP8266 (Sensors for temperature and gas) 
- **Navigation:** Transbot mobile platform with line tracking & obstacle avoidance  
- **Localization:** ArTag visual markers  
- **User Interface:** PC-based UI with alert system + mobile notifications (Telegram) 

---

## ⚙️ Installation & Setup

### Requirements
- NVIDIA Jetson Nano (4GB recommended)  
- Transbot robot platform  
- Ubuntu 18.04  with JetPack SDK  
- Anaconda / Miniconda  
- YOLOv5 (official repo)
- arduino （ESP8266 NodeMCU）
- - Jetson Nano and PC（which show the dashboard） must be connected to the **same network**   

### 🚀 First-Time Setup Guide

This guide describes the **complete startup procedure** for the multimodal warehouse inspection robot  
(Fire/Smoke detection, ArTag localization, Lidar, Line following, Gas & Temperature sensing, Cloud integration, and Alerts).
#### 1️⃣ Train YOLOv5 Model (PC with GPU)
1. Clone YOLOv5:
   ```bash
   git clone https://github.com/ultralytics/yolov5.git
   cd yolov5
2. Install dependencies:
   ```bash
   pip install -r requirements.txt
3. Train your dataset:
   ```bash
   python train.py --img 640 --batch 16 --epochs 100 --data data.yaml --weights yolov5s.pt
   
4. Export weights (best.pt) for deployment.

✅ Alternatively, you can skip training and directly use the pretrained fire & smoke detection model from this repository:
[fire_and_smoke_detection_yolov5](https://github.com/weilinhan/fire_and_smoke_detection_yolov5)

####  2️⃣ Deploy YOLOv5 on Jetson Nano
1. Install Anaconda on Jetson Nano.
2. Create environment:
     ```bash
    conda create -n yolo_env python=3.7.1 -y
    conda activate yolo_env
  
3. Clone YOLOv5 on Jetson Nano and install dependencies:
    ```bash
    git clone https://github.com/ultralytics/yolov5.git
    cd yolov5
    pip install -r requirements.txt

4. Copy your trained weights (best.pt) to this folder.

#### 3️⃣ Configure AWS IoT

1. Create a **Thing** in the [AWS IoT Core Console](https://console.aws.amazon.com/iot).  
2. Download the device certificates:
   - `certificate.pem.crt`  
   - `private.pem.key`  
   - `AmazonRootCA1.pem`  
3. Place the certificates in the following folders:  
   - `yolov5/` → for **YOLO fire detection**  
   - `src/transbot_visual/scripts/` → for **ArTag localization**  
4. Update your AWS configuration file `config/aws_config.json` with your endpoint and certificate paths, e.g.:

    ```json
    {
      "endpoint": "your-aws-endpoint.amazonaws.com",
      "port": 8883,
      "rootCA": "aws_certificates/AmazonRootCA1.pem",
      "certfile": "aws_certificates/certificate.pem.crt",
      "keyfile": "aws_certificates/private.pem.key",
      "topic": "fire/detection"
    }

#### 4️⃣ Start YOLO Fire/Smoke Detection

Open **Terminal A** on Jetson Nano and run:

    ```bash
    cd yolov5
    conda activate yolo_env
    python detect_fire_mqtt.py \
      --weights yolov5s_best.pt \
      --source 1 \
      --view-img \
      --ros \
      --ros-caminfo-yaml ~/.ros/camera_info/camera.yaml
**Arguments:**

- `--weights` → Path to the trained YOLOv5 model (e.g., `yolov5s_best.pt`).  
  This file should be trained on your PC and then copied to Jetson Nano.  

- `--source` → Video source:  
  - `0` = default camera  
  - `1` = USB camera  
  - or path to a video file (e.g., `video.mp4`)  

- `--view-img` → Display the detection results in a window.  

- `--ros` → Enable ROS publishing of detection results.  

- `--ros-caminfo-yaml` → Path to the ROS camera calibration file  
  (default: `~/.ros/camera_info/camera.yaml`).  
#### 5️⃣ Start ArTag Localization

Open **Terminal B** and run:

    ```bash
    conda deactivate
    roslaunch transbot_visual ar_track.launch

#### 6️⃣ Start Lidar

Open **Terminal C** and run:

    ```bash
    conda deactivate
    roslaunch transbot_nav laser_bringup.launch

#### 7️⃣ Start Line Following

Open **Terminal D** and run:

    ```bash
    conda deactivate
    roslaunch transbot_linefollow follow_line.launch VideoSwitch:=true

#### 8️⃣ Start Gas & Temperature Sensors (ESP8266/Arduino)

1. Go to the `arduino/` folder and open the sketch in **Arduino IDE**.  
2. Update your Wi-Fi credentials in the code if you are in first time use it:
   ```bash
   const char* ssid = "YOUR_WIFI_NAME";
   const char* password = "YOUR_WIFI_PASSWORD";
   
3. Upload the code to your ESP8266 board (e.g., NodeMCU or Wemos D1 mini).
    
4. Place the AWS IoT certificates (certificate.pem.crt, private.pem.key, AmazonRootCA1.pem) in the same folder as the Arduino sketch.
    
5. Power on the board.

**Arguments:**

(configured inside the Arduino sketch: Wi-Fi SSID, Wi-Fi password, AWS IoT certificates)

Expected result:

The ESP8266 connects to your Wi-Fi network.

Gas concentration and temperature readings are collected.

Sensor data is published to AWS IoT Core via MQTT.

You can verify the data in the AWS IoT MQTT test client under topics like sensor/gas and sensor/temperature.
#### 9️⃣ Verify Data on AWS IoT

1. Go to the **AWS IoT Console → MQTT Test Client**.  
2. Subscribe to the following topics:  
   - `yolo/fire`  for yolo detection
   - `esp8266/pub`  for temperature and smoke detection
   - `transbot/tag`  for artag localization
 

**Arguments:**
- *(no extra arguments required; only AWS IoT topics need to be subscribed in the console)*

**Expected result:**
- You will see JSON payloads arriving in real time.  


#### 🔟 Dashboard & Telegram Alerts

To visualize data and receive notifications, you need to start both the **Dashboard frontend** and the **MQTT backend server**.

---

**Step 1. Run Dashboard frontend**

      ```bash
      cd dashboard-app
      npm install   # only required the first time
      npm run dev

**Step 2. Run MQTT backend server**

      ```bash
      cd mqtt-server
      npm install   # only required the first time
      node server.js
  
**Arguments:**
  - Certificates required in this folder:  
  - `AmazonRootCA1.pem`  
  - `certificate.pem.crt`  
  - `private.pem.key`  

**Expected result:**
  - Connects to **AWS IoT Core** using certificates.  
  - Subscribes to topics:  
  - `yolo/fire`  
  - `esp8266/pub`  
  - `transbot/tag`    
  - Forwards MQTT messages to the dashboard frontend via WebSocket.  
  - Ensures dashboard updates in **real time**.
  - 
**Step 3. Telegram Bot Alerts**
Use a Telegram bot to receive **instant alerts** and **fire images** on your phone.

---

  **1) Create a Telegram Bot (via BotFather)**
  
  1. Open Telegram and search for **@BotFather**.  
  2. Send `/start`, then `/newbot`.  
  3. Follow prompts to set a **name** and a unique **username** (must end with `bot`, e.g., `warehouse_fire_bot`).  
  4. BotFather will return a **Bot Token** like `123456789:ABC-DEF1234ghIkl-zyx57W2v1u123abcDe`.  
  5. (Optional) Set bot description and photo:  
     - `/setdescription`, `/setabouttext`, `/setuserpic`  
  6. (Recommended) Disable privacy to receive all messages in groups (optional):  
     - `/setprivacy` → choose your bot → **Disable**
  
  > 🔐 **Keep the Bot Token secret.** Do not commit it to the repo.
  
  ---
  
  **2) Configure the backend to send alerts**
  
  Create a `.env` file (or edit your config) in the **mqtt-server** folder:
  
    ```env
    TELEGRAM_BOT_TOKEN=123456789:ABC-DEF1234ghIkl-zyx57W2v1u123abcDe
    TELEGRAM_CHAT_ID=YOUR_CHAT_ID


## 🎥 Demo

-Fire/smoke detection with YOLOv5 (insert screenshot/video)

-Transbot autonomous navigation (photo or GIF)

-PC UI showing inspection data & alerts (screenshot)
## 📊 Results

-Reliable early fire detection in real-time

-Low computational overhead with Jetson Nano + optimized YOLOv5

-Fully autonomous navigation on predefined inspection routes

-Applicable for real warehouse environments with limited infrastructure
