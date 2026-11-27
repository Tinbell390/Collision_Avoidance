"""
exp2_monitor.py

実験2用のシリアルモニタ

必要ライブラリ
    pip install pyserial
"""
import csv
import queue
import os
import sys
import subprocess

import threading
try:
    from .serial_common import SerialManager as SharedSerialManager
except ImportError:
    from serial_common import SerialManager as SharedSerialManager
from datetime import datetime

import tkinter as tk
from tkinter import ttk, messagebox
from tkinter.scrolledtext import ScrolledText

import matplotlib
matplotlib.use("TkAgg")
from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import (
    FigureCanvasTkAgg,
    NavigationToolbar2Tk
)


# =====================================================
# パス設定
# =====================================================
# このファイルは monitor/ に置かれている想定。
# ログは log/ に、解析プログラムは tool/exp2_analysis.py にあるものとし、
# 実行時のカレントディレクトリに依存しないよう、このファイル自身の場所を
# 基準に絶対パスを組み立てる。
_SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
_PROJECT_ROOT = os.path.dirname(_SCRIPT_DIR)

LOG_DIR = os.path.join(_PROJECT_ROOT, "log")
ANALYSIS_SCRIPT = os.path.join(_PROJECT_ROOT, "tool", "exp2_analysis.py")


# =====================================================
# SerialManager
# =====================================================
class SerialManager(SharedSerialManager):
    def __init__(self):
        super().__init__()
        self.plot_queue = queue.Queue()
        self.receiving_csv = False
        self.current_vehicle = None
        self.csv_buffer = []
        self.expected_count = None
        self.received_count = 0

    def get_plot_request(self):
        try:
            return self.plot_queue.get_nowait()
        except queue.Empty:
            return None

    # START
    # -------------------------------------------------
    def send_start(self):

        self.send("start lonery")

    # -------------------------------------------------
    # FINISH
    # -------------------------------------------------
    def send_finish(self):

        self.send("finish")

    # -------------------------------------------------
    # SET SPEED
    # -------------------------------------------------
    def send_speed(self, speed):

        self.send(f"speed {speed}")

    # -------------------------------------------------
    # SET GAIN
    # -------------------------------------------------
    def send_gain(self, kp, ki, kd):
        self.send(f"gain {kp} {ki} {kd}")

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

        os.makedirs(LOG_DIR, exist_ok=True)

        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")

        filename = os.path.join(
            LOG_DIR,
            f"{timestamp}_vehicle{vehicle}.csv"
        )
        # Avoid overwriting a log created within the same timestamp second.
        suffix = 1
        while os.path.exists(filename):
            filename = os.path.join(LOG_DIR, f"{timestamp}_vehicle{vehicle}_{suffix}.csv")
            suffix += 1
        temporary = filename + ".tmp"

        try:

            with open(temporary, "w", encoding="utf-8", newline="") as f:
                for line in self.csv_buffer:
                    f.write(line + "\n")
            os.replace(temporary, filename)

            self.log(f"Saved : {filename}")

            # 保存が終わったらGUI側でグラフ表示できるように通知
            self.plot_queue.put(filename)

            # 保存したログを解析プログラム(tool/exp2_analysis.py)に渡して解析する
            self.launch_analysis(filename)

        except Exception as e:

            try:
                if os.path.exists(temporary):
                    os.remove(temporary)
            except OSError:
                pass
            self.log(f"Save Error : {e}")

    # -------------------------------------------------
    # 解析プログラムの起動 (finishで得たログを解析する)
    # -------------------------------------------------
    def launch_analysis(self, filename):

        if not os.path.isfile(ANALYSIS_SCRIPT):
            self.log(f"Analysis Error : 解析プログラムが見つかりません ({ANALYSIS_SCRIPT})")
            return

        try:

            subprocess.Popen(
                [sys.executable, ANALYSIS_SCRIPT, os.path.abspath(filename)],
                cwd=os.path.dirname(ANALYSIS_SCRIPT)
            )

            self.log(f"Analyzing : {filename}")

        except Exception as e:

            self.log(f"Analysis Launch Error : {e}")


# =====================================================
# MonitorGUI
# =====================================================
class MonitorGUI:

    #==================================================
    # 初期化
    #==================================================
    def __init__(self):

        self.serial = SerialManager()

        # 直近にSETしたPIDゲイン (グラフのタイトル表示用)
        self.last_gain = {"kp": None, "ki": None, "kd": None}

        self.root = tk.Tk()
        self.root.title("ESP32 Monitor")
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
            width=55,
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
        # 操作ボタン
        #------------------------------
        button_frame = ttk.Frame(self.root)
        button_frame.pack(fill="x", padx=10)

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
        # 目標速度設定
        #------------------------------
        speed_frame = ttk.Frame(self.root)
        speed_frame.pack(fill="x", padx=10, pady=10)

        ttk.Label(
            speed_frame,
            text="Target Speed"
        ).pack(side="left")

        self.speed_var = tk.StringVar(value="100")

        self.speed_entry = ttk.Entry(
            speed_frame,
            width=10,
            textvariable=self.speed_var
        )

        self.speed_entry.pack(side="left", padx=5)

        ttk.Button(
            speed_frame,
            text="SET SPEED",
            command=self.send_speed
        ).pack(side="left", padx=5)

        # ------------------------------
        # PIDゲイン設定
        # ------------------------------
        gain_frame = ttk.Frame(self.root)
        gain_frame.pack(fill="x", padx=10, pady=5)

        ttk.Label(
            gain_frame,
            text="KP"
        ).pack(side="left")

        self.kp_var = tk.StringVar(value="1.0")
        ttk.Entry(
            gain_frame,
            width=8,
            textvariable=self.kp_var
        ).pack(side="left", padx=3)

        ttk.Label(
            gain_frame,
            text="KI"
        ).pack(side="left")

        self.ki_var = tk.StringVar(value="0.0")
        ttk.Entry(
            gain_frame,
            width=8,
            textvariable=self.ki_var
        ).pack(side="left", padx=3)

        ttk.Label(
            gain_frame,
            text="KD"
        ).pack(side="left")

        self.kd_var = tk.StringVar(value="0.0")
        ttk.Entry(
            gain_frame,
            width=8,
            textvariable=self.kd_var
        ).pack(side="left", padx=3)

        ttk.Button(
            gain_frame,
            text="SET GAIN",
            command=self.send_gain
        ).pack(side="left", padx=10)

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

        selected = self.port_var.get()
        ports = self.serial.get_ports()

        self.port_box["values"] = ports

        if selected in ports:
            self.port_var.set(selected)
        elif len(ports) > 0:
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
        if self.serial.connect(port_label, expected_id="monitor"):
            self.connect_button.config(text="Connecting...", state="disabled")

    def disconnect(self):
        self.serial.disconnect()
        self.connect_button.config(text="Connect", command=self.connect, state="normal")


    #==================================================
    # START
    #==================================================
    def send_start(self):

        self.serial.send_start()

    #==================================================
    # FINISH
    #==================================================
    def send_finish(self):

        self.serial.send_finish()

    #==================================================
    # SET SPEED
    #==================================================
    def send_speed(self):

        try:
            speed = int(self.speed_var.get())
        except ValueError:
            self.append_log("Invalid speed")
            return

        self.serial.send_speed(speed)

    #==================================================
    # SET GAIN
    #==================================================
    def send_gain(self):

        try:
            kp = float(self.kp_var.get())
            ki = float(self.ki_var.get())
            kd = float(self.kd_var.get())
        except ValueError:
            self.append_log("Invalid gain")
            return

        # グラフタイトル表示用に直近のゲインを保持
        self.last_gain = {"kp": kp, "ki": ki, "kd": kd}

        self.serial.send_gain(kp, ki, kd)

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

        # 保存されたCSVがあればグラフウィンドウを開く
        while True:

            filepath = self.serial.get_plot_request()

            if filepath is None:
                break

            self.show_graph(filepath)

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
    # グラフ表示
    # (Time_us を横軸に Speed_cm_s / Target_speed_cm_s / PWM を
    #  1つのグラフにまとめて表示する)
    #==================================================
    def show_graph(self, filepath):

        # 表示したい列名 (CSVヘッダーと照合するキー)
        columns = {
            "time_us": "Time_us",
            "speed_cm_s": "Speed_cm_s",
            "target_speed_cm_s": "Target_speed_cm_s",
            "pwm": "PWM",
        }

        data = {key: [] for key in columns}

        row_count = 0
        skip_count = 0

        try:

            # utf-8-sig : 先頭にBOMが付いていても取り除いて読める
            with open(filepath, "r", encoding="utf-8-sig", newline="") as f:

                reader = csv.reader(f)

                header = next(reader, None)

                if header is None:
                    self.append_log(f"Graph Read Error : Empty file ({filepath})")
                    return

                # 列名前後の空白を除去し、大小文字を無視してインデックスを探す
                header_clean = [h.strip() for h in header]
                header_lower = [h.lower() for h in header_clean]

                idx = {}

                try:
                    for key, name in columns.items():
                        idx[key] = header_lower.index(name.lower())
                except ValueError:
                    self.append_log(
                        f"Graph Read Error : 必要な列が見つかりません "
                        f"(検出したヘッダー: {header_clean})"
                    )
                    return

                max_idx = max(idx.values())

                for row in reader:

                    row_count += 1

                    if len(row) <= max_idx:
                        skip_count += 1
                        continue

                    try:
                        values = {
                            key: float(row[i].strip())
                            for key, i in idx.items()
                        }
                    except ValueError:
                        skip_count += 1
                        continue

                    for key in columns:
                        data[key].append(values[key])

        except Exception as e:
            self.append_log(f"Graph Read Error : {e}")
            return

        if len(data["time_us"]) == 0:
            self.append_log(
                f"Graph Data Empty : {filepath} "
                f"(header={header_clean}, rows_read={row_count}, skipped={skip_count})"
            )
            return

        if skip_count > 0:
            self.append_log(
                f"Graph Warning : {skip_count}/{row_count} rows skipped in {filepath}"
            )

        # ------------------------------
        # タイトル用のゲイン文字列
        # ------------------------------
        gain = self.last_gain

        if gain["kp"] is None:
            gain_text = "KP=?, KI=?, KD=?"
        else:
            gain_text = f"KP={gain['kp']}, KI={gain['ki']}, KD={gain['kd']}"

        # ------------------------------
        # 別ウィンドウ作成
        # ------------------------------
        graph_win = tk.Toplevel(self.root)
        graph_win.title(f"Graph - {os.path.basename(filepath)}")
        graph_win.geometry("800x600")

        fig = Figure(figsize=(7, 5.5), dpi=100)
        ax = fig.add_subplot(111)

        # Speed / Target Speed は左軸 (cm/s)
        line1, = ax.plot(
            data["time_us"], data["speed_cm_s"],
            label="Speed_cm_s", color="tab:blue", linewidth=2.2
        )
        line2, = ax.plot(
            data["time_us"], data["target_speed_cm_s"],
            label="Target_speed_cm_s", color="tab:orange",
            linestyle="--", linewidth=2.2
        )

        ax.set_xlabel("Time_us")
        ax.set_ylabel("Speed [cm/s],PWM")
        ax.grid(True)

        line3, = ax.plot(
            data["time_us"], data["pwm"],
            label="PWM", color="yellow", linewidth=2.5, alpha=0.9
        )

        lines = [line1, line2, line3]
        ax.legend(lines, [l.get_label() for l in lines], loc="upper right")

        ax.set_title(
            f"{os.path.basename(filepath)}\n{gain_text}"
        )

        fig.tight_layout()

        canvas = FigureCanvasTkAgg(fig, master=graph_win)
        canvas.draw()
        canvas.get_tk_widget().pack(fill="both", expand=True)

        toolbar = NavigationToolbar2Tk(canvas, graph_win)
        toolbar.update()
        canvas.get_tk_widget().pack(fill="both", expand=True)

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