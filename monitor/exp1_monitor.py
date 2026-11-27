"""
exp1_monitor.py

実験1用のシリアルモニタ

必要ライブラリ
    pip install pyserial
"""
from serial_common import SerialManager as SharedSerialManager

import tkinter as tk
from tkinter import ttk, messagebox
from tkinter.scrolledtext import ScrolledText


# =====================================================
# SerialManager
# =====================================================
SerialManager = SharedSerialManager


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
        self.port_box.bind("<Button-1>", lambda _event: self.refresh_ports())

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
        if self.serial.connected:
            self.disconnect()
            return
        if self.serial.connecting:
            return
        port_label = self.port_var.get()
        if not port_label:
            return
        if "(not recommended)" in port_label and not messagebox.askyesno(
                "Unrecognized serial device",
                "This port is not in the recommended USB-serial list. Continue anyway?"):
            return
        if self.serial.connect(port_label, expected_id="espnow"):
            self.connect_button.config(text="Connecting...", state="disabled")

    def disconnect(self):
        self.serial.disconnect()
        self.connect_button.config(text="Connect", command=self.connect, state="normal")


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
    # ログ更新
    #==================================================
    def update_log(self):

        while True:
            event = self.serial.get_event()
            if event is None:
                break
            event_type, detail = event
            if event_type == "connected":
                self.connect_button.config(text="Disconnect", command=self.disconnect, state="normal")
            elif event_type in ("connect_failed", "disconnected"):
                self.connect_button.config(text="Connect", command=self.connect, state="normal")
                if event_type == "connect_failed":
                    messagebox.showwarning("Serial connection failed", detail)

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