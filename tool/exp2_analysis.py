"""
exp2_analysis.py

実験2 (単独走行・PIDゲイン調整) ログ解析プログラム

--------------------------------------------------------------------
概要
--------------------------------------------------------------------
exp2_monitor.py が保存したCSVログ (Time_us, Speed_cm_s, Target_speed_cm_s,
PWM, Line_count, EnterTime_us, ExitTime_us) を読み込み、下記9項目の解析を
行って output/<タイムスタンプ>/ 以下に figure/ (グラフ画像) と report.md
(解析結果のレポート) を出力する。

  1. speed / target speed / PWM の時系列
  2. 速度誤差 (目標速度 - 実速度)
  3. MAE・RMSE
  4. 整定時間
  5. オーバーシュート
  6. P/I/D の推移 (P=e, I=Σe, D=Δe。係数不明のため未乗算。
     本コントローラはフレームごとの計算でdtを用いないため、
     I=誤差の累積和、D=誤差の変化量として時系列で図示する)
  7. EnterTime の成功率
  8. absEnterTime (= Time_us + EnterTime_us) の平均・標準偏差
  9. absExitTime (= Time_us + ExitTime_us) の平均・標準偏差

--------------------------------------------------------------------
使い方
--------------------------------------------------------------------
    python exp2_analysis.py [CSVファイルパス]

exp2_monitor.py から呼び出すことを想定し、実行時引数はCSVファイルパス
1つのみを受け取る。引数が無い場合は default_file を使用する。
解析完了後、report.mdの内容と図を表示するビューアウィンドウが自動で
ポップアップする(SHOW_REPORT_VIEWER = False にすると表示しない)。

必要ライブラリ
    numpy, matplotlib  (matplotlib.use("Agg") によりGUI無しでも動作する)
    tkinter (ビューア表示用。標準ライブラリに含まれる)
"""

import sys
import os
import re
import csv
import math

import numpy as np
import matplotlib
matplotlib.use("Agg")  # 画面表示せずファイル保存のみ行うため
import matplotlib.pyplot as plt


# =====================================================================
# 設定 (必要に応じて調整する)
# =====================================================================

# 引数なしで実行した場合に使用するファイル
default_file = "log/20260827_135529_vehicle0.csv"

# このスクリプト自身の場所 (tool/ を想定)。呼び出し元のカレントディレクトリに
# 依存せず、常にプロジェクトルートの output/ に結果を出力するための基準。
_SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))

# 出力先ルートディレクトリ (tool/ と同じ階層の output/ = ../output)
OUTPUT_ROOT = os.path.join(_SCRIPT_DIR, "..", "output")

# --- 整定時間・オーバーシュート判定のパラメータ -------------------------
# 整定判定の許容誤差 (目標速度に対する割合。例: 0.05 = ±5%)
SETTLING_TOLERANCE_RATIO = 0.05
# 許容範囲内に留まり続けたとみなす最小継続時間 [s]
SETTLING_HOLD_TIME_S = 0.3
# 目標速度がこの値以上変化した点を「ステップ変化」とみなす [cm/s]
# (センサ/通信ノイズによる微小な目標値ゆらぎを無視するため)
TARGET_STEP_MIN_DIFF = 1.0

# オーバーシュートを評価する「立ち上がり区間」の最大長 [s]
# ステップ後、目標値の許容範囲に一度も入らないまま区間が長く続く場合でも、
# この時間を超えた後の変動(再度の乱れ等)はオーバーシュートに含めない。
TRANSIENT_WINDOW_S = 2.0

# 解析完了後にレポートビューアのウィンドウを自動表示するかどうか
SHOW_REPORT_VIEWER = True

# ビューア内で画像を表示する際の最大幅 [px] (これを超える場合は整数倍で縮小する)
VIEWER_MAX_IMAGE_WIDTH = 820


# =====================================================================
# ユーティリティ
# =====================================================================

def extract_timestamp(csv_path):
    """
    ファイル名から時刻部分 (YYYYMMDD_HHMMSS) を抽出する。
    見つからない場合は拡張子を除いたファイル名をそのまま使う。
    """
    base = os.path.basename(csv_path)
    m = re.match(r"(\d{8}_\d{6})", base)
    if m:
        return m.group(1)
    return os.path.splitext(base)[0]


def to_float_or_nan(value):
    value = value.strip()
    if value == "":
        return float("nan")
    try:
        return float(value)
    except ValueError:
        return float("nan")


def load_csv(csv_path):
    """
    CSVを読み込みnumpy配列の辞書として返す。
    EnterTime_us / ExitTime_us が空欄の行はnanとする。
    """
    time_us, speed, target, pwm, line_count = [], [], [], [], []
    enter_time, exit_time = [], []

    with open(csv_path, "r", encoding="utf-8-sig", newline="") as f:
        reader = csv.DictReader(f)

        # 列名の前後空白を無視して照合できるようにする
        reader.fieldnames = [h.strip() for h in reader.fieldnames]

        for row in reader:
            row = {k.strip(): v for k, v in row.items()}

            if row.get("Time_us", "").strip() == "":
                continue

            time_us.append(float(row["Time_us"]))
            speed.append(to_float_or_nan(row.get("Speed_cm_s", "")))
            target.append(to_float_or_nan(row.get("Target_speed_cm_s", "")))
            pwm.append(to_float_or_nan(row.get("PWM", "")))
            line_count.append(to_float_or_nan(row.get("Line_count", "")))
            enter_time.append(to_float_or_nan(row.get("EnterTime_us", "")))
            exit_time.append(to_float_or_nan(row.get("ExitTime_us", "")))

    data = {
        "time_us": np.array(time_us, dtype=float),
        "speed": np.array(speed, dtype=float),
        "target": np.array(target, dtype=float),
        "pwm": np.array(pwm, dtype=float),
        "line_count": np.array(line_count, dtype=float),
        "enter_time": np.array(enter_time, dtype=float),
        "exit_time": np.array(exit_time, dtype=float),
    }

    if len(data["time_us"]) == 0:
        raise ValueError(f"有効なデータ行が見つかりませんでした: {csv_path}")

    return data


# =====================================================================
# 解析: 1. 時系列 / 2. 速度誤差
# =====================================================================

def plot_speed_pwm(time_s, speed, target, pwm, fig_path):
    fig, ax1 = plt.subplots(figsize=(9, 5))

    l1, = ax1.plot(time_s, speed, label="Speed [cm/s]", color="tab:blue", linewidth=1.8)
    l2, = ax1.plot(time_s, target, label="Target speed [cm/s]", color="tab:orange",
                    linestyle="--", linewidth=1.8)
    ax1.set_xlabel("Time [s]")
    ax1.set_ylabel("Speed [cm/s]")
    ax1.grid(True, alpha=0.3)

    ax2 = ax1.twinx()
    l3, = ax2.plot(time_s, pwm, label="PWM", color="tab:green", alpha=0.6, linewidth=1.2)
    ax2.set_ylabel("PWM")

    lines = [l1, l2, l3]
    ax1.legend(lines, [l.get_label() for l in lines], loc="upper right")
    ax1.set_title("Speed / Target speed / PWM")

    fig.tight_layout()
    fig.savefig(fig_path, dpi=150)
    plt.close(fig)


def plot_error(time_s, error, fig_path):
    fig, ax = plt.subplots(figsize=(9, 4))
    ax.plot(time_s, error, color="tab:red", linewidth=1.5)
    ax.axhline(0, color="black", linewidth=0.8)
    ax.set_xlabel("Time [s]")
    ax.set_ylabel("Speed error (target - speed) [cm/s]")
    ax.set_title("Speed error")
    ax.grid(True, alpha=0.3)
    fig.tight_layout()
    fig.savefig(fig_path, dpi=150)
    plt.close(fig)


# =====================================================================
# 解析: 3. MAE / RMSE
# =====================================================================

def compute_mae_rmse(error):
    valid = error[~np.isnan(error)]
    mae = float(np.mean(np.abs(valid)))
    rmse = float(np.sqrt(np.mean(valid ** 2)))
    return mae, rmse


# =====================================================================
# 解析: 4. 整定時間 / 5. オーバーシュート
# =====================================================================

def find_target_segments(time_s, target):
    """
    目標速度が (ノイズを除き) 一定とみなせる区間ごとに分割する。
    実験2では基本的に区間は1つ (開始から目標速度が変化しない) になる。
    実験3のように途中で目標速度が変わるデータにも対応できるようにしている。

    戻り値: [(start_idx, end_idx, target_value), ...]  end_idxは区間末尾(含む)
    """
    segments = []
    start_idx = 0
    current_target = target[0]

    for i in range(1, len(target)):
        if abs(target[i] - current_target) >= TARGET_STEP_MIN_DIFF:
            segments.append((start_idx, i - 1, current_target))
            start_idx = i
            current_target = target[i]

    segments.append((start_idx, len(target) - 1, current_target))
    return segments


def compute_overshoot_pct(seg_t0, seg_speed, tgt, step):
    """
    古典的なオーバーシュートの定義に基づき算出する:
      1. ステップ後、応答が初めて目標値に到達(通過)する時刻を求める
      2. その後、応答が再び目標値を逆向きに通過するまで(=最初の1山/1谷)の
         区間内での最大(または最小)値をピークとする
      3. 目標値に到達しないまま TRANSIENT_WINDOW_S を超えた場合はそこで打ち切る
    これにより、立ち上がり後しばらく経ってから生じる別要因の乱れを
    オーバーシュートとして誤カウントしないようにしている。
    """
    diff = seg_speed - tgt

    if step > 0:
        reached = np.where(diff >= 0)[0]
    else:
        reached = np.where(diff <= 0)[0]

    if len(reached) == 0:
        # 観測区間内で目標値に到達していない場合、オーバーシュートは評価不能(0とする)
        return 0.0

    cross_idx = int(reached[0])
    limit_time = seg_t0[cross_idx] + TRANSIENT_WINDOW_S

    end_idx = len(seg_t0)
    for k in range(cross_idx + 1, len(seg_t0)):
        if seg_t0[k] > limit_time:
            end_idx = k
            break
        if step > 0 and diff[k] < 0:
            end_idx = k
            break
        if step < 0 and diff[k] > 0:
            end_idx = k
            break

    window = seg_speed[cross_idx:end_idx]

    if step > 0:
        peak = np.max(window)
        return max(0.0, (peak - tgt) / step * 100.0)
    else:
        trough = np.min(window)
        return max(0.0, (tgt - trough) / abs(step) * 100.0)


def analyze_step_response(time_s, speed, target, segments):
    """
    各区間について整定時間・オーバーシュートを計算する。
    最初の区間は「静止状態(実測初速)からのステップ」として扱い、
    2区間目以降は「直前区間の目標速度からのステップ」として扱う。
    """
    results = []

    for seg_i, (s, e, tgt) in enumerate(segments):
        if seg_i == 0:
            ref_value = speed[s]  # 実測の初速を基準にする
        else:
            prev_tgt = segments[seg_i - 1][2]
            ref_value = prev_tgt

        step = tgt - ref_value
        seg_t = time_s[s:e + 1]
        seg_speed = speed[s:e + 1]
        seg_t0 = seg_t - seg_t[0]

        # --- オーバーシュート -------------------------------------------------
        if abs(step) < 1e-6:
            overshoot_pct = None
        else:
            overshoot_pct = compute_overshoot_pct(seg_t0, seg_speed, tgt, step)

        # --- 整定時間 -----------------------------------------------------
        band = SETTLING_TOLERANCE_RATIO * max(abs(tgt), 1.0)
        within = np.abs(seg_speed - tgt) <= band

        settling_time = None
        # 「seg_t0[k]以降ずっとwithinがTrue」となる最小のkを探す
        for k in range(len(seg_t0)):
            if np.all(within[k:]):
                # 継続時間がSETTLING_HOLD_TIME_S以上であることを確認
                if (seg_t0[-1] - seg_t0[k]) >= SETTLING_HOLD_TIME_S or k == 0:
                    settling_time = seg_t0[k]
                    break

        results.append({
            "segment": seg_i + 1,
            "start_time_s": float(seg_t[0]),
            "end_time_s": float(seg_t[-1]),
            "target": float(tgt),
            "step": float(step),
            "overshoot_pct": overshoot_pct,
            "settling_time_s": settling_time,
            "settled": settling_time is not None,
        })

    return results


# =====================================================================
# 解析: 6. P/I/D 寄与率 (proxy)
# =====================================================================

def analyze_pid_contribution(time_s, error):
    """
    P項(e) / I項(Σe) / D項(Δe) の時系列を求める。

    このPIDコントローラはフレームごとの計算を行っており、dtを用いない
    (I項は誤差をそのまま累積、D項は変化量をdtで割らない)仕様に合わせ、
    ここでもdtは使用しない。
    実際のKp, Ki, Kdは不明なため、各項は係数を掛ける前の生の値である。
    """
    p_term = error
    i_term = np.cumsum(error)
    d_term = np.diff(error, prepend=error[0])

    return {
        "p_term": p_term, "i_term": i_term, "d_term": d_term,
    }


def plot_pid_timeseries(time_s, pid_result, fig_path):
    """
    P項(e) / I項(Σe) / D項(Δe) それぞれの時系列を並べて表示する。
    3項でスケールが大きく異なる場合があるため、上下3段のサブプロットとする。
    """
    fig, axes = plt.subplots(3, 1, figsize=(9, 8), sharex=True)

    axes[0].plot(time_s, pid_result["p_term"], color="tab:blue", linewidth=1.3)
    axes[0].set_ylabel("P: e")
    axes[0].axhline(0, color="black", linewidth=0.6)
    axes[0].grid(True, alpha=0.3)

    axes[1].plot(time_s, pid_result["i_term"], color="tab:orange", linewidth=1.3)
    axes[1].set_ylabel("I: \u03a3e")
    axes[1].axhline(0, color="black", linewidth=0.6)
    axes[1].grid(True, alpha=0.3)

    axes[2].plot(time_s, pid_result["d_term"], color="tab:green", linewidth=1.3)
    axes[2].set_ylabel("D: \u0394e")
    axes[2].axhline(0, color="black", linewidth=0.6)
    axes[2].grid(True, alpha=0.3)
    axes[2].set_xlabel("Time [s]")

    fig.suptitle("P / I / D term time series (raw, dt not applied)")
    fig.tight_layout()
    fig.savefig(fig_path, dpi=150)
    plt.close(fig)


# =====================================================================
# 解析: 7. EnterTimeの成功率 / 8. absEnterTime / 9. absExitTime
# =====================================================================

def analyze_enter_exit(time_us, line_count, enter_time, exit_time):
    n_total = len(time_us)

    enter_valid_mask = ~np.isnan(enter_time)
    exit_valid_mask = ~np.isnan(exit_time)

    n_enter_valid = int(np.sum(enter_valid_mask))
    n_exit_valid = int(np.sum(exit_valid_mask))

    # 行単位の成功率 (EnterTime_us が記録された行の割合)
    enter_success_rate_rows = n_enter_valid / n_total * 100.0 if n_total > 0 else float("nan")

    # Line_count(周回/区間)単位での成功率
    # (同一Line_countの中で1回でもEnterTime_usが記録されていれば成功とみなす)
    unique_lines = np.unique(line_count[~np.isnan(line_count)])
    n_lines_total = len(unique_lines)
    n_lines_success = 0
    for lc in unique_lines:
        mask = (line_count == lc)
        if np.any(enter_valid_mask & mask):
            n_lines_success += 1
    enter_success_rate_lines = (
        n_lines_success / n_lines_total * 100.0 if n_lines_total > 0 else float("nan")
    )

    # absEnterTime / absExitTime
    if n_enter_valid > 0:
        abs_enter = time_us[enter_valid_mask] + enter_time[enter_valid_mask]
        abs_enter_mean = float(np.mean(abs_enter))
        abs_enter_std = float(np.std(abs_enter))
    else:
        abs_enter = np.array([])
        abs_enter_mean = float("nan")
        abs_enter_std = float("nan")

    if n_exit_valid > 0:
        abs_exit = time_us[exit_valid_mask] + exit_time[exit_valid_mask]
        abs_exit_mean = float(np.mean(abs_exit))
        abs_exit_std = float(np.std(abs_exit))
    else:
        abs_exit = np.array([])
        abs_exit_mean = float("nan")
        abs_exit_std = float("nan")

    return {
        "n_total_rows": n_total,
        "n_enter_valid": n_enter_valid,
        "n_exit_valid": n_exit_valid,
        "enter_success_rate_rows": enter_success_rate_rows,
        "n_lines_total": n_lines_total,
        "n_lines_success": n_lines_success,
        "enter_success_rate_lines": enter_success_rate_lines,
        "abs_enter_mean": abs_enter_mean,
        "abs_enter_std": abs_enter_std,
        "abs_exit_mean": abs_exit_mean,
        "abs_exit_std": abs_exit_std,
        "abs_enter_values": abs_enter,
        "abs_exit_values": abs_exit,
    }


def plot_enter_exit(time_us, abs_enter_values, abs_exit_values, fig_path):
    """
    absEnterTime/absExitTimeの時系列上でのばらつきを可視化する。
    """
    if len(abs_enter_values) == 0 and len(abs_exit_values) == 0:
        return False

    fig, ax = plt.subplots(figsize=(9, 4))
    if len(abs_enter_values) > 0:
        ax.scatter(np.arange(len(abs_enter_values)), abs_enter_values / 1e6,
                   label="absEnterTime [s]", color="tab:purple", s=14)
    if len(abs_exit_values) > 0:
        ax.scatter(np.arange(len(abs_exit_values)), abs_exit_values / 1e6,
                   label="absExitTime [s]", color="tab:brown", s=14)
    ax.set_xlabel("Sample index (valid rows only)")
    ax.set_ylabel("Absolute predicted time [s]")
    ax.set_title("absEnterTime / absExitTime (valid samples)")
    ax.legend()
    ax.grid(True, alpha=0.3)
    fig.tight_layout()
    fig.savefig(fig_path, dpi=150)
    plt.close(fig)
    return True


# =====================================================================
# レポート生成
# =====================================================================

def fmt(value, digits=2, unit=""):
    if value is None or (isinstance(value, float) and np.isnan(value)):
        return "N/A"
    return f"{value:.{digits}f}{unit}"


def build_report(csv_path, out_dir, data, mae, rmse, step_results, pid_result,
                  enter_exit_result, has_enter_exit_fig):
    lines = []
    lines.append(f"# 実験2 解析レポート")
    lines.append("")
    lines.append(f"- 入力ファイル: `{os.path.basename(csv_path)}`")
    lines.append(f"- データ点数: {len(data['time_us'])}")
    lines.append(f"- 計測時間: {(data['time_us'][-1] - data['time_us'][0]) / 1e6:.2f} s")
    lines.append("")

    # 1. 時系列
    lines.append("## 1. Speed / Target speed / PWM")
    lines.append("")
    lines.append("![speed_pwm](figure/01_speed_pwm.png)")
    lines.append("")

    # 2. 速度誤差
    lines.append("## 2. 速度誤差 (target - speed)")
    lines.append("")
    lines.append("![error](figure/02_error.png)")
    lines.append("")

    # 3. MAE/RMSE
    lines.append("## 3. MAE・RMSE")
    lines.append("")
    lines.append(f"- MAE  : {fmt(mae, 3)} cm/s")
    lines.append(f"- RMSE : {fmt(rmse, 3)} cm/s")
    lines.append("")

    # 4/5. 整定時間・オーバーシュート
    lines.append("## 4. 整定時間 / 5. オーバーシュート")
    lines.append("")
    lines.append(
        f"判定条件: 目標速度に対し ±{SETTLING_TOLERANCE_RATIO * 100:.0f}% "
        f"の範囲に {SETTLING_HOLD_TIME_S:.1f}s 以上留まり続けた時点を整定時間とする。"
    )
    lines.append("")
    lines.append("| 区間 | 開始[s] | 終了[s] | 目標速度[cm/s] | ステップ量[cm/s] | 整定時間[s] | オーバーシュート[%] |")
    lines.append("|---|---|---|---|---|---|---|")
    for r in step_results:
        settling_str = fmt(r["settling_time_s"], 2) if r["settled"] else "未整定"
        overshoot_str = fmt(r["overshoot_pct"], 1) if r["overshoot_pct"] is not None else "N/A"
        lines.append(
            f"| {r['segment']} | {r['start_time_s']/1e0:.2f} | {r['end_time_s']:.2f} | "
            f"{r['target']:.1f} | {r['step']:.1f} | {settling_str} | {overshoot_str} |"
        )
    lines.append("")
    if len(step_results) > 1:
        lines.append(
            "※ このデータでは目標速度が途中で変化していますが、実験2の走行データは"
            "目標速度が一定のため通常は区間1つのみとなります。"
        )
        lines.append("")

    # 6. PID各項の時系列
    lines.append("## 6. P/I/D の推移")
    lines.append("")
    lines.append(
        "実際のKp, Ki, Kdは既知ではないため、各項は係数を掛ける前の生の値である。"
        "本PIDコントローラはフレームごとの計算を行っており、"
        "I項は誤差をそのまま累積(Σe)、D項は変化量をdtで割らない(Δe)仕様のため、"
        "本解析でもdtは用いていない。"
    )
    lines.append("")
    lines.append("![pid_timeseries](figure/03_pid_timeseries.png)")
    lines.append("")

    # 7. entertime成功率
    lines.append("## 7. EnterTime の成功率")
    lines.append("")
    ee = enter_exit_result
    lines.append(
        f"- 行単位: {ee['n_enter_valid']} / {ee['n_total_rows']} 行 "
        f"({fmt(ee['enter_success_rate_rows'], 1)}%)"
    )
    lines.append(
        f"- Line_count単位: {ee['n_lines_success']} / {ee['n_lines_total']} 区間 "
        f"({fmt(ee['enter_success_rate_lines'], 1)}%)"
    )
    lines.append(
        "  (Line_count単位は、同一Line_countの区間内で1回でもEnterTime_usが"
        "記録されていれば成功とみなした値)"
    )
    lines.append("")

    # 8/9. absEnterTime / absExitTime
    lines.append("## 8. absEnterTime (= Time_us + EnterTime_us)")
    lines.append("")
    lines.append(f"- 平均   : {fmt(ee['abs_enter_mean'], 1)} us")
    lines.append(f"- 標準偏差 : {fmt(ee['abs_enter_std'], 1)} us")
    lines.append(f"- 有効データ数 : {ee['n_enter_valid']}")
    lines.append("")

    lines.append("## 9. absExitTime (= Time_us + ExitTime_us)")
    lines.append("")
    lines.append(f"- 平均   : {fmt(ee['abs_exit_mean'], 1)} us")
    lines.append(f"- 標準偏差 : {fmt(ee['abs_exit_std'], 1)} us")
    lines.append(f"- 有効データ数 : {ee['n_exit_valid']}")
    lines.append("")

    if has_enter_exit_fig:
        lines.append("absEnterTime / absExitTime の推移 (有効サンプルのみ):")
        lines.append("")
        lines.append("![enter_exit](figure/04_enter_exit.png)")
        lines.append("")

    report_text = "\n".join(lines)

    with open(os.path.join(out_dir, "report.md"), "w", encoding="utf-8") as f:
        f.write(report_text)


# =====================================================================
# レポートビューア (tkinterポップアップ)
# =====================================================================

_MD_IMAGE_PATTERN = re.compile(r"^!\[[^\]]*\]\(([^)]+)\)$")


def show_report_viewer(out_dir):
    """
    report.md の内容と図を1つのウィンドウにまとめて表示する簡易ビューア。
    Markdownの厳密なレンダリングは行わず、見出し(#, ##)・画像(![]())・
    テーブル行(|...|)・通常テキストを簡易的に区別して表示する。
    """
    import tkinter as tk
    from tkinter.scrolledtext import ScrolledText

    report_path = os.path.join(out_dir, "report.md")
    if not os.path.isfile(report_path):
        print(f"report.mdが見つからないため、ビューアは表示しません: {report_path}")
        return

    with open(report_path, "r", encoding="utf-8") as f:
        md_lines = f.read().splitlines()

    root = tk.Tk()
    root.title(f"実験2 解析結果 - {os.path.basename(out_dir)}")
    root.geometry("900x850")

    text = ScrolledText(root, wrap="word", font=("", 10))
    text.pack(fill="both", expand=True, padx=4, pady=4)

    text.tag_configure("h1", font=("", 18, "bold"), spacing1=6, spacing3=10)
    text.tag_configure("h2", font=("", 14, "bold"), spacing1=14, spacing3=6)
    text.tag_configure("body", font=("", 10))
    text.tag_configure("mono", font=("Courier", 9))

    # 画像のPhotoImage参照 (GCで消えないようウィジェットにぶら下げておく)
    text._image_refs = []

    for line in md_lines:
        m = _MD_IMAGE_PATTERN.match(line.strip())

        if m:
            img_rel_path = m.group(1)
            img_path = os.path.normpath(os.path.join(out_dir, img_rel_path))

            if os.path.isfile(img_path):
                try:
                    img = tk.PhotoImage(file=img_path)

                    if img.width() > VIEWER_MAX_IMAGE_WIDTH:
                        factor = math.ceil(img.width() / VIEWER_MAX_IMAGE_WIDTH)
                        img = img.subsample(factor, factor)

                    text._image_refs.append(img)
                    text.image_create("end", image=img)
                    text.insert("end", "\n\n")
                except Exception as e:
                    text.insert("end", f"[画像読み込み失敗: {img_rel_path} ({e})]\n\n", "body")
            else:
                text.insert("end", f"[画像が見つかりません: {img_rel_path}]\n\n", "body")
            continue

        if line.startswith("# "):
            text.insert("end", line[2:] + "\n\n", "h1")
        elif line.startswith("## "):
            text.insert("end", line[3:] + "\n\n", "h2")
        elif line.strip().startswith("|"):
            text.insert("end", line + "\n", "mono")
        elif line.strip() == "":
            text.insert("end", "\n")
        else:
            text.insert("end", line + "\n", "body")

    text.configure(state="disabled")

    root.mainloop()


# =====================================================================
# メイン処理
# =====================================================================

def analyze(csv_path):
    data = load_csv(csv_path)

    time_us = data["time_us"]
    time_s = (time_us - time_us[0]) / 1e6
    speed = data["speed"]
    target = data["target"]
    pwm = data["pwm"]
    error = target - speed

    ts = extract_timestamp(csv_path)
    out_dir = os.path.normpath(os.path.join(OUTPUT_ROOT, ts))
    fig_dir = os.path.join(out_dir, "figure")
    os.makedirs(fig_dir, exist_ok=True)

    # 1 / 2 : 図の生成
    plot_speed_pwm(time_s, speed, target, pwm, os.path.join(fig_dir, "01_speed_pwm.png"))
    plot_error(time_s, error, os.path.join(fig_dir, "02_error.png"))

    # 3 : MAE / RMSE
    mae, rmse = compute_mae_rmse(error)

    # 4 / 5 : 整定時間・オーバーシュート
    segments = find_target_segments(time_s, target)
    step_results = analyze_step_response(time_s, speed, target, segments)

    # 6 : PID各項の時系列
    pid_result = analyze_pid_contribution(time_s, error)
    plot_pid_timeseries(time_s, pid_result, os.path.join(fig_dir, "03_pid_timeseries.png"))

    # 7 / 8 / 9 : entertime / exittime
    enter_exit_result = analyze_enter_exit(
        time_us, data["line_count"], data["enter_time"], data["exit_time"]
    )
    has_enter_exit_fig = plot_enter_exit(
        time_us,
        enter_exit_result["abs_enter_values"],
        enter_exit_result["abs_exit_values"],
        os.path.join(fig_dir, "04_enter_exit.png"),
    )

    build_report(
        csv_path, out_dir, data, mae, rmse, step_results, pid_result,
        enter_exit_result, has_enter_exit_fig,
    )

    print(f"解析完了: {out_dir}")
    print(f"  MAE={mae:.3f} cm/s, RMSE={rmse:.3f} cm/s")
    for r in step_results:
        settled = f"{r['settling_time_s']:.2f}s" if r["settled"] else "未整定"
        overshoot_str = "N/A" if r["overshoot_pct"] is None else f"{r['overshoot_pct']:.1f}%"
        print(
            f"  区間{r['segment']}: target={r['target']:.1f}cm/s, "
            f"settling={settled}, overshoot={overshoot_str}"
        )

    return out_dir


def main():
    if len(sys.argv) >= 2:
        csv_path = sys.argv[1]
    else:
        csv_path = default_file

    if not os.path.isfile(csv_path):
        print(f"エラー: ファイルが見つかりません: {csv_path}")
        sys.exit(1)

    out_dir = analyze(csv_path)

    if SHOW_REPORT_VIEWER:
        try:
            show_report_viewer(out_dir)
        except Exception as e:
            print(f"レポートビューアの表示に失敗しました (解析結果自体は{out_dir}に保存済みです): {e}")


if __name__ == "__main__":
    main()