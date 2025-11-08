#!/usr/bin/env python3
"""
main.py — simple TCP client for ESP32
------------------------------------
Connects to ESP32 running a WiFiServer on a fixed IP (default: 192.168.1.200, port 3333)
Sends messages typed by the user and prints responses.

You can later import this file or extend it with functions to
send sensor data, structured commands, etc.
"""

import socket
import sys
import time


# ==== Configuration ====
ESP_HOST = "10.100.102.101"   # static IP of your ESP32
ESP_PORT = 8000              # must match the Arduino server port
RECV_BUFFER = 4096
TIMEOUT = 10                 # seconds for initial connection


# ==== TCP Client Class ====
class ESP32Client:
    def __init__(self, host=ESP_HOST, port=ESP_PORT):
        self.host = host
        self.port = port
        self.sock = None

    def connect(self):
        """Open TCP connection to ESP32."""
        print(f"Connecting to {self.host}:{self.port} ...")
        try:
            self.sock = socket.create_connection((self.host, self.port), timeout=TIMEOUT)
            print("✅ Connected to ESP32.")
            self.sock.settimeout(1.0)
            # Read initial greeting if any
            try:
                data = self.sock.recv(RECV_BUFFER)
                if data:
                    print(data.decode(errors="ignore").rstrip())
            except socket.timeout:
                pass
        except OSError as e:
            print(f"❌ Connection failed: {e}")
            sys.exit(1)

    def send(self, msg: str):
        """Send a single line message to ESP32 and print its reply."""
        if not self.sock:
            print("⚠️  Not connected.")
            return
        try:
            self.sock.sendall((msg + "\n").encode())
            try:
                data = self.sock.recv(RECV_BUFFER)
                if data:
                    print(data.decode(errors="ignore").rstrip())
            except socket.timeout:
                print("(no reply)")
        except OSError as e:
            print(f"❌ Send failed: {e}")
            self.close()

    def close(self):
        if self.sock:
            try:
                self.sock.close()
            except OSError:
                pass
            self.sock = None
            print("🔌 Connection closed.")


# ==== Main interactive loop ====
def main():
    client = ESP32Client()
    client.connect()

    print("Type messages to send to ESP32. Ctrl+C or empty line to quit.")
    try:
        while True:
            msg = input("> ").strip()
            if not msg:
                break
            client.send(msg)
            time.sleep(0.1)
    except KeyboardInterrupt:
        pass
    finally:
        client.close()


if __name__ == "__main__":
    main()
