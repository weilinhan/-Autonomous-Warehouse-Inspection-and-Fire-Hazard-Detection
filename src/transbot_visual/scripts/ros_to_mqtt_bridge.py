#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import ssl
import json
import rospy
import paho.mqtt.client as mqtt
from ar_track_alvar.msg import AlvarMarkers

# ================= MQTT AWS IoT =================
MQTT_HOST = "a1dtwlsw1olgol-ats.iot.eu-central-1.amazonaws.com"
MQTT_PORT = 8883
MQTT_TOPIC = "transbot/tag"

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
CA_PATH   = os.path.join(SCRIPT_DIR, "AmazonRootCA1.pem")
CERT_PATH = os.path.join(SCRIPT_DIR, "certificate.pem.crt")
KEY_PATH  = os.path.join(SCRIPT_DIR, "private.pem.key")

print("[MQTT] CA_PATH   =", CA_PATH)
print("[MQTT] CERT_PATH =", CERT_PATH)
print("[MQTT] KEY_PATH  =", KEY_PATH)

# =================  MQTT  =================
mqtt_client = mqtt.Client(client_id="ros-artag-bridge", clean_session=True)
mqtt_client.tls_set(
    ca_certs=CA_PATH,
    certfile=CERT_PATH,
    keyfile=KEY_PATH,
    tls_version=ssl.PROTOCOL_TLSv1_2
)

def _on_connect(c, u, f, rc):
    rospy.loginfo("MQTT connected rc=%s", rc)

def _on_disconnect(c, u, rc):
    rospy.logwarn("MQTT disconnected rc=%s", rc)

def _on_publish(c, u, mid):
    rospy.logdebug("MQTT publish ack mid=%s", mid)

mqtt_client.on_connect = _on_connect
mqtt_client.on_disconnect = _on_disconnect
mqtt_client.on_publish = _on_publish
mqtt_client.reconnect_delay_set(min_delay=1, max_delay=8)

mqtt_client.connect(MQTT_HOST, MQTT_PORT, keepalive=60)
mqtt_client.loop_start()  

# ================ ROS pub tag =================
def ar_callback(msg):
    for marker in msg.markers:
        tag_id = int(marker.id)
        payload = {"type": "tag", "id": tag_id}
        #  QoS=1 
        info = mqtt_client.publish(MQTT_TOPIC, json.dumps(payload), qos=1)
        info.wait_for_publish()
        if info.rc != mqtt.MQTT_ERR_SUCCESS:
            rospy.logerr("Publish failed rc=%s", info.rc)
        else:
            rospy.loginfo("Published tag ID %s to MQTT", tag_id)

def main():
    rospy.init_node("ros_to_mqtt_bridge", anonymous=True)
    rospy.Subscriber("/ar_pose_marker", AlvarMarkers, ar_callback, queue_size=10)
    rospy.loginfo("ROS to MQTT bridge started. Topic: %s", MQTT_TOPIC)
    try:
        rospy.spin()
    finally:
        try:
            mqtt_client.loop_stop()
            mqtt_client.disconnect()
        except Exception:
            pass

if __name__ == "__main__":
    main()

