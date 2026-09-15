"""
exp0_monitor.py

ESP32-C3 動作確認用シリアルモニタ

Python -> ESP32-C3 : HELLO
ESP32-C3 -> Python : HELLO FROM ESP32-C3

必要ライブラリ
    pip install pyserial
"""

import queue
import serial
import serial.tools.list_ports

import threading

import tkinter as tk
from tkinter import ttk
from tkinter.scrolledtext import ScrolledText


# =====================================================
# SerialManager
# =====================================================
class SerialManager:

    def __init__(self):

        self.ser = None

        self.running = False

        self.receive_thread = None

        # GUIへ渡すメッセージ
        self.message_queue = queue.Queue()

    # -------------------------------------------------
    # GUIへログ表示
    # -------------------------------------------------
    def log(self, text):

        print(text)

        self.message_queue.put(text)

    # -------------------------------------------------
    # GUI用メッセージ取得
    # -------------------------------------------------
    def get_message(self):

        try:
            return self.message_queue.get_nowait()

        except queue.Empty:
            return None

    # -------------------------------------------------
    # COMポート一覧取得
    # -------------------------------------------------
    @staticmethod
    def get_ports():

        ports = serial.tools.list_ports.comports()

        return [port.device for port in ports]

    # -------------------------------------------------
    # 接続
    # -------------------------------------------------
    def connect(self, port, baudrate=115200):

        if self.ser is not None:
            return True

        try:

            self.ser = serial.Serial(
                port=port,
                baudrate=baudrate,
                timeout=0.1
            )

            self.running = True

            self.receive_thread = threading.Thread(
                target=self.receive_loop,
                daemon=True
            )

            self.receive_thread.start()

            self.log(f"Connected : {port}")

            return True

        except Exception as e:

            self.log(f"Connection Error : {e}")

            return False

    # -------------------------------------------------
    # 切断
    # -------------------------------------------------
    def disconnect(self):

        self.running = False

        if self.ser is not None:

            try:
                self.ser.close()

            except:
                pass

            self.ser = None

        self.log("Disconnected")

    # -------------------------------------------------
    # HELLO送信
    # -------------------------------------------------
    def send_hello(self):

        if self.ser is None:
            self.log("Not connected")

            return

        try:

            self.ser.write(b"HELLO\n")

            self.log("> HELLO")

        except Exception as e:

            self.log(f"Send Error : {e}")

    # -------------------------------------------------
    # 受信スレッド
    # -------------------------------------------------
    def receive_loop(self):

        while self.running:

            if self.ser is None:
                break

            try:

                line = self.ser.readline()

                if len(line) == 0:
                    continue

                text = line.decode(
                    errors="ignore"
                ).strip()

                if text == "":
                    continue

                self.log("< " + text)

            except Exception as e:

                if self.running:
                    self.log(f"Receive Error : {e}")

                break


# =====================================================
# MonitorGUI
# =====================================================
class MonitorGUI:

    # =================================================
    # 初期化
    # =================================================
    def __init__(self):

        self.serial = SerialManager()

        self.root = tk.Tk()

        self.root.title("ESP32-C3 Serial Test")

        self.root.geometry("600x450")

        # -------------------------------------------------
        # COMポート
        # -------------------------------------------------
        top = ttk.Frame(self.root)

        top.pack(
            fill="x",
            padx=10,
            pady=10
        )

        ttk.Label(
            top,
            text="COM Port"
        ).pack(side="left")

        self.port_var = tk.StringVar()

        self.port_box = ttk.Combobox(
            top,
            width=20,
            textvariable=self.port_var,
            state="readonly"
        )

        self.port_box.pack(
            side="left",
            padx=5
        )

        ttk.Button(
            top,
            text="Refresh",
            command=self.refresh_ports
        ).pack(side="left")

        self.connect_button = ttk.Button(
            top,
            text="Connect",
            command=self.connect
        )

        self.connect_button.pack(
            side="left",
            padx=10
        )

        # -------------------------------------------------
        # HELLOボタン
        # -------------------------------------------------
        hello_frame = ttk.Frame(self.root)

        hello_frame.pack(
            fill="x",
            padx=10,
            pady=10
        )

        ttk.Button(
            hello_frame,
            text="Send HELLO",
            command=self.send_hello,
            width=20
        ).pack()

        # -------------------------------------------------
        # ログ
        # -------------------------------------------------
        ttk.Label(
            self.root,
            text="Log"
        ).pack(
            anchor="w",
            padx=10
        )

        self.log = ScrolledText(
            self.root,
            height=18,
            state="disabled"
        )

        self.log.pack(
            fill="both",
            expand=True,
            padx=10,
            pady=10
        )

        # -------------------------------------------------
        # COM一覧取得
        # -------------------------------------------------
        self.refresh_ports()

        # -------------------------------------------------
        # Queue監視開始
        # -------------------------------------------------
        self.root.after(
            50,
            self.update_log
        )

        # -------------------------------------------------
        # ×ボタン
        # -------------------------------------------------
        self.root.protocol(
            "WM_DELETE_WINDOW",
            self.close
        )

    # =================================================
    # COM一覧更新
    # =================================================
    def refresh_ports(self):

        ports = self.serial.get_ports()

        self.port_box["values"] = ports

        if len(ports) > 0:

            self.port_box.current(0)

    # =================================================
    # 接続
    # =================================================
    def connect(self):

        if self.serial.ser is None:

            port = self.port_var.get()

            if port == "":
                return

            if self.serial.connect(port):

                self.connect_button.config(
                    text="Disconnect",
                    command=self.disconnect
                )

        else:

            self.disconnect()

    # =================================================
    # 切断
    # =================================================
    def disconnect(self):

        self.serial.disconnect()

        self.connect_button.config(
            text="Connect",
            command=self.connect
        )

    # =================================================
    # HELLO
    # =================================================
    def send_hello(self):

        self.serial.send_hello()

    # =================================================
    # ログ更新
    # =================================================
    def update_log(self):

        while True:

            message = self.serial.get_message()

            if message is None:
                break

            self.append_log(message)

        self.root.after(
            50,
            self.update_log
        )

    # =================================================
    # ログ追加
    # =================================================
    def append_log(self, text):

        self.log.configure(
            state="normal"
        )

        self.log.insert(
            tk.END,
            text + "\n"
        )

        self.log.see(
            tk.END
        )

        self.log.configure(
            state="disabled"
        )

    # =================================================
    # 終了処理
    # =================================================
    def close(self):

        try:
            self.serial.disconnect()

        except:
            pass

        self.root.destroy()

    # =================================================
    # GUI開始
    # =================================================
    def run(self):

        self.root.mainloop()


# =====================================================
# main
# =====================================================
def main():

    app = MonitorGUI()

    app.run()


if __name__ == "__main__":
    main()