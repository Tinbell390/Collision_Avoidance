"""
exp3_monitor.py

実験3用シリアルモニタ

必要ライブラリ
    pip install pyserial
"""
import queue
import serial
import os
import serial.tools.list_ports

import threading
from datetime import datetime

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

        # CSV受信
        self.receiving_csv = False
        self.current_vehicle = None
        self.csv_buffer = []

        # 転送予定ファイル数の管理
        self.expected_count = None
        self.received_count = 0


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
    # コマンド送信
    # -------------------------------------------------
    def send(self, command):

        if self.ser is None:
            return

        try:

            self.ser.write((command + "\n").encode())

            self.log("> " + command)

        except Exception as e:

            self.log(f"Send Error : {e}")

    # -------------------------------------------------
    # START（衝突フラグに応じて分岐）
    # -------------------------------------------------
    def send_start(self, collision):

        if collision:
            self.send("start collision")
        else:
            self.send("start")

    # -------------------------------------------------
    # FINISH
    # -------------------------------------------------
    def send_finish(self):

        self.send("finish")

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

                # -------------------------------
                # 転送予定件数
                # -------------------------------
                if text.startswith("COUNT"):

                    parts = text.split()

                    if len(parts) == 2:
                        self.expected_count = int(parts[1])
                    else:
                        self.expected_count = None

                    self.received_count = 0

                    self.log(f"Expecting {self.expected_count} log file(s)")

                    continue


                # -------------------------------
                # CSV開始
                # -------------------------------
                if text.startswith("BEGIN"):

                    parts = text.split()

                    if len(parts) == 2:
                        self.current_vehicle = int(parts[1])
                    else:
                        self.current_vehicle = 0

                    self.receiving_csv = True
                    self.csv_buffer.clear()

                    self.log(f"Receiving Vehicle {self.current_vehicle}")

                    continue

                # -------------------------------
                # CSV終了
                # -------------------------------
                if text.startswith("END"):

                    self.save_csv(self.current_vehicle)

                    self.receiving_csv = False
                    self.current_vehicle = None

                    self.received_count += 1

                    # 予定件数すべて受信したらACKを返す
                    if (self.expected_count is not None
                            and self.received_count >= self.expected_count):

                        self.send("ACK")
                        self.expected_count = None
                        self.received_count = 0

                    continue

                # -------------------------------
                # 全転送終了
                # -------------------------------
                if text == "ALL LOGS DELETED":
                    self.log(text)
                    continue

                # -------------------------------
                # CSV受信中
                # -------------------------------
                if self.receiving_csv:
                    self.csv_buffer.append(text)
                else:
                    self.log(text)

            except Exception as e:

                self.log(f"Receive Error : {e}")

                self.disconnect()

                break

    # -------------------------------------------------
    # CSV保存
    # -------------------------------------------------
    def save_csv(self, vehicle):

        os.makedirs("log", exist_ok=True)

        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")

        filename = os.path.join(
            "log",
            f"{timestamp}_vehicle{vehicle}.csv"
        )

        try:

            with open(filename, "w", encoding="utf-8") as f:

                for line in self.csv_buffer:
                    f.write(line + "\n")

            self.log(f"Saved : {filename}")

        except Exception as e:

            self.log(f"Save Error : {e}")


# =====================================================
# MonitorGUI
# =====================================================
class MonitorGUI:

    #==================================================
    # 初期化
    #==================================================
    def __init__(self):

        self.serial = SerialManager()

        self.root = tk.Tk()
        self.root.title("ESP32 Monitor - Experiment 3")
        self.root.geometry("700x550")

        #------------------------------
        # COMポート
        #------------------------------
        top = ttk.Frame(self.root)
        top.pack(fill="x", padx=10, pady=10)

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

        self.port_box.pack(side="left", padx=5)

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

        self.connect_button.pack(side="left", padx=10)

        #------------------------------
        # 衝突フラグ（トグルスイッチ）
        #------------------------------
        collision_frame = ttk.Frame(self.root)
        collision_frame.pack(fill="x", padx=10, pady=5)

        ttk.Label(
            collision_frame,
            text="Collision Flag"
        ).pack(side="left")

        self.collision_var = tk.BooleanVar(value=False)

        self.collision_switch = ttk.Checkbutton(
            collision_frame,
            text="OFF",
            variable=self.collision_var,
            command=self.toggle_collision
        )

        self.collision_switch.pack(side="left", padx=10)

        #------------------------------
        # 操作ボタン
        #------------------------------
        button_frame = ttk.Frame(self.root)
        button_frame.pack(fill="x", padx=10, pady=10)

        self.start_button = ttk.Button(
            button_frame,
            text="START",
            command=self.send_start,
            width=25
        )

        self.start_button.pack(pady=5)

        self.finish_button = ttk.Button(
            button_frame,
            text="FINISH",
            command=self.send_finish,
            width=25
        )

        self.finish_button.pack(pady=5)

        #------------------------------
        # ログ表示
        #------------------------------
        ttk.Label(
            self.root,
            text="Log"
        ).pack(anchor="w", padx=10)

        self.log = ScrolledText(
            self.root,
            height=20,
            state="disabled"
        )

        self.log.pack(
            fill="both",
            expand=True,
            padx=10,
            pady=10
        )

        # COM一覧取得
        self.refresh_ports()

        # Queue監視開始
        self.root.after(
            50,
            self.update_log
        )

        # ×ボタン
        self.root.protocol(
            "WM_DELETE_WINDOW",
            self.close
        )

    #==================================================
    # COM一覧更新
    #==================================================
    def refresh_ports(self):

        ports = self.serial.get_ports()

        self.port_box["values"] = ports

        if len(ports) > 0:
            self.port_box.current(0)

    #==================================================
    # 接続
    #==================================================
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

    #==================================================
    # 切断
    #==================================================
    def disconnect(self):

        self.serial.disconnect()

        self.connect_button.config(
            text="Connect",
            command=self.connect
        )

    #==================================================
    # 衝突フラグ切替表示
    #==================================================
    def toggle_collision(self):

        if self.collision_var.get():
            self.collision_switch.config(text="ON")
        else:
            self.collision_switch.config(text="OFF")

    #==================================================
    # START（衝突フラグの状態を反映）
    #==================================================
    def send_start(self):

        self.serial.send_start(self.collision_var.get())

    #==================================================
    # FINISH
    #==================================================
    def send_finish(self):

        self.serial.send_finish()

    #==================================================
    # ログ更新
    #==================================================
    def update_log(self):

        while True:

            message = self.serial.get_message()

            if message is None:
                break

            self.append_log(message)

        # 50ms後に再度チェック
        self.root.after(50, self.update_log)

    #==================================================
    # ログ追加
    #==================================================
    def append_log(self, text):

        self.log.configure(state="normal")

        self.log.insert(tk.END, text + "\n")

        self.log.see(tk.END)

        self.log.configure(state="disabled")

    #==================================================
    # 終了処理
    #==================================================
    def close(self):

        try:
            self.serial.disconnect()
        except:
            pass

        self.root.destroy()

    #==================================================
    # GUI開始
    #==================================================
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