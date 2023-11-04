import os
import sys
import logging
import paho.mqtt.client as mqtt
import json
import csv
import random
import base64
import binascii
from datetime import datetime
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from datetime import datetime, timedelta

script_dir = os.path.dirname(os.path.abspath(__file__))
os.chdir(script_dir)

USER = "lora-test-ful@ttn"
PASSWORD = "NNSXS.43M3AY73ECK6MFPXQ3X5THOV27SAY224UJXBUOQ.4JO563N3PQHPHLHOYYBYCVHMNFCYXNPNPAOTSLUNVMPNKND763RA"
PUBLIC_TLS_ADDRESS = "eu1.cloud.thethings.network"
PUBLIC_TLS_ADDRESS_PORT = 8883
DEVICE_ID = "eui-2cf7f12032304b2d"
ALL_DEVICES = True
DOWNLINKS_FILE = "downlinks.txt"
last_sent_value = None

# Clear the content of downlinks.txt at the beginning
with open(DOWNLINKS_FILE, 'w') as f:
    f.write('')

# Initialize the last_sent_value with the last line of downlinks.txt at the beginning of the script
try:
    with open(DOWNLINKS_FILE, 'r') as f:
        lines = f.readlines()
        last_sent_value = lines[-1].strip() if lines else None
except FileNotFoundError:
    last_sent_value = None

QOS = 0

DEBUG = True

def decode_payload(encoded_payload):
    try:
        decoded_bytes = base64.b64decode(encoded_payload)
        decoded_str = decoded_bytes.decode('ascii')
        return decoded_str
    except binascii.Error as e:
        print(f"Failed to decode base64: {e}")
    except UnicodeDecodeError as e:
        print(f"Decoded bytes are not a valid ASCII string: {e}")
    return None

def get_value_from_json_object(obj, key):
    try:
        return obj[key]
    except KeyError:
        return '-'

def stop(client):
    client.disconnect()
    print("\nExit")
    sys.exit(0)

def save_to_file(some_json):
    end_device_ids = some_json["end_device_ids"]
    device_id = end_device_ids["device_id"]
    application_id = end_device_ids["application_ids"]["application_id"]

    if 'uplink_message' in some_json:
        uplink_message = some_json["uplink_message"]
        f_port = get_value_from_json_object(uplink_message, "f_port")

        if f_port != '-':
            current_time = datetime.now().strftime("%H:%M")
            frm_payload_encoded = uplink_message["frm_payload"]
            frm_payload = decode_payload(frm_payload_encoded)
            path_n_file = "resultat.txt"
            print(path_n_file)
            
            last_time = None
            data_to_write = f"{current_time} {frm_payload}\n"
            
            if os.path.exists(path_n_file) and os.path.getsize(path_n_file) > 0:
                with open(path_n_file, 'r') as txtFile:
                    lines = txtFile.readlines()
                    last_line = lines[-1]
                    last_time = last_line.split()[0]
            
            if last_time == current_time:
                data_to_write = lines[-1].strip() + frm_payload + "\n"
                lines[-1] = data_to_write
                with open(path_n_file, 'w', newline='') as txtFile:
                    txtFile.writelines(lines)
            else:
                with open(path_n_file, 'a', newline='') as txtFile:
                    txtFile.write(data_to_write)

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("\nConnected successfully to MQTT broker")
    else:
        print("\nFailed to connect, return code = " + str(rc))

def on_message(client, userdata, message):
    print("\nMessage received on topic '" + message.topic + "' with QoS = " + str(message.qos))
    parsed_json = json.loads(message.payload)
    if DEBUG:
        print("Payload (Collapsed): " + str(message.payload))
        print("Payload (Expanded): \n" + json.dumps(parsed_json, indent=4))
    save_to_file(parsed_json)

def on_subscribe(client, userdata, mid, granted_qos):
    print("\nSubscribed with message id (mid) = " + str(mid) + " and QoS = " + str(granted_qos))

def on_disconnect(client, userdata, rc):
    print("\nDisconnected with result code = " + str(rc))

def on_log(client, userdata, level, buf):
    print("\nLog: " + buf)
    logging_level = client.LOGGING_LEVEL[level]
    logging.log(logging_level, buf)

def get_last_line_from_file(filename):
    with open(filename, 'r') as f:
        lines = f.readlines()
        return lines[-1].strip() if lines else None

def monitor_downlinks_file():
    global last_sent_value
    current_value = get_last_line_from_file(DOWNLINKS_FILE)
    if current_value and current_value != last_sent_value:
        print(f"New value detected: {current_value}")
        send_downlink(current_value)
        last_sent_value = current_value

def send_downlink(payload_str):
    encoded_payload = base64.b64encode(payload_str.encode('ascii')).decode('ascii')
    downlink_msg = {
        "downlinks": [{
            "f_port": 1,
            "frm_payload": encoded_payload
        }]
    }

    client.publish(f"v3/{USER}/devices/{DEVICE_ID}/down/replace", json.dumps(downlink_msg))

client = mqtt.Client(client_id=f'python-mqtt-{random.randint(0, 1000)}')

client.username_pw_set(username=USER, password=PASSWORD)
client.tls_set()
client.on_connect = on_connect
client.on_message = on_message
client.on_subscribe = on_subscribe
client.on_disconnect = on_disconnect

if DEBUG:
    client.on_log = on_log

client.connect(PUBLIC_TLS_ADDRESS, PUBLIC_TLS_ADDRESS_PORT)
client.subscribe(f'v3/{USER}/devices/{DEVICE_ID}/up', qos=QOS)

try:
    run = True
    while run:
        client.loop(10)
        print(".", end="", flush=True)
        monitor_downlinks_file()
except KeyboardInterrupt:
    stop(client)
