"""
exp1_monitor.py

実験1用のシリアルモニタ

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
    # HELLO送信（ブロードキャスト）
    # -------------------------------------------------
    def send_hello(self):

        self.send("HELLO")

    # -------------------------------------------------
    # Peer登録
    # -------------------------------------------------
    def send_addpeer(self, mac):

        self.send(f"addpeer {mac}")

    # -------------------------------------------------
    # PING送信
    # -------------------------------------------------
    def send_ping(self, mac):

        self.send(f"PING {mac}")

    # -------------------------------------------------
    # DATA_REQ送信
    # -------------------------------------------------
    def send_datareq(self, mac):

        self.send(f"DATAREQ {mac}")

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

                self.log(text)

            except Exception as e:

                self.log(f"Receive Error : {e}")

                self.disconnect()

                break


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
        self.root.title("ESP-NOW Monitor (実験1)")
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
        # HELLO（ブロードキャスト）
        #------------------------------
        hello_frame = ttk.Frame(self.root)
        hello_frame.pack(fill="x", padx=10, pady=5)

        ttk.Button(
            hello_frame,
            text="HELLO (Broadcast)",
            command=self.send_hello,
            width=25
        ).pack(side="left")

        #------------------------------
        # MACアドレス指定 + 各種送信
        #------------------------------
        mac_frame = ttk.Frame(self.root)
        mac_frame.pack(fill="x", padx=10, pady=10)

        ttk.Label(
            mac_frame,
            text="MAC Address"
        ).pack(side="left")

        self.mac_var = tk.StringVar(value="XX:XX:XX:XX:XX:XX")

        self.mac_entry = ttk.Entry(
            mac_frame,
            width=20,
            textvariable=self.mac_var
        )

        self.mac_entry.pack(side="left", padx=5)

        button_frame = ttk.Frame(self.root)
        button_frame.pack(fill="x", padx=10)

        ttk.Button(
            button_frame,
            text="Add Peer",
            command=self.send_addpeer,
            width=15
        ).pack(side="left", padx=5, pady=5)

        ttk.Button(
            button_frame,
            text="PING",
            command=self.send_ping,
            width=15
        ).pack(side="left", padx=5, pady=5)

        ttk.Button(
            button_frame,
            text="DATA REQUEST",
            command=self.send_datareq,
            width=15
        ).pack(side="left", padx=5, pady=5)

        #------------------------------
        # 任意コマンド送信
        #------------------------------
        raw_frame = ttk.Frame(self.root)
        raw_frame.pack(fill="x", padx=10, pady=10)

        ttk.Label(
            raw_frame,
            text="Command"
        ).pack(side="left")

        self.raw_var = tk.StringVar()

        self.raw_entry = ttk.Entry(
            raw_frame,
            width=40,
            textvariable=self.raw_var
        )

        self.raw_entry.pack(side="left", padx=5)
        self.raw_entry.bind("<Return>", lambda e: self.send_raw())

        ttk.Button(
            raw_frame,
            text="Send",
            command=self.send_raw
        ).pack(side="left", padx=5)

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
    # HELLO
    #==================================================
    def send_hello(self):

        self.serial.send_hello()

    #==================================================
    # Add Peer
    #==================================================
    def send_addpeer(self):

        mac = self.mac_var.get().strip()

        if mac == "":
            self.append_log("Invalid MAC Address")
            return

        self.serial.send_addpeer(mac)

    #==================================================
    # PING
    #==================================================
    def send_ping(self):

        mac = self.mac_var.get().strip()

        if mac == "":
            self.append_log("Invalid MAC Address")
            return

        self.serial.send_ping(mac)

    #==================================================
    # DATA REQUEST
    #==================================================
    def send_datareq(self):

        mac = self.mac_var.get().strip()

        if mac == "":
            self.append_log("Invalid MAC Address")
            return

        self.serial.send_datareq(mac)

    #==================================================
    # 任意コマンド送信
    #==================================================
    def send_raw(self):

        command = self.raw_var.get().strip()

        if command == "":
            return

        self.serial.send(command)

        self.raw_var.set("")

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