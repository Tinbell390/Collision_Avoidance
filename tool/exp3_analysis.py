#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
analyze_exp3.py

実験3（衝突回避走行）のログCSVを解析し，report.md にまとめて出力するプログラム。
exp3_monitor.py から，走行終了後の後処理として呼び出されることを想定している。

【実行時引数】
  実行時引数として受け取るのはファイル名のみ（他のオプションは持たない）。

  0個: 引数を渡さない場合は DEFAULT_FILES に格納されたファイル名を使用する。
       python analyze_exp3.py

  1個: ファイル名を1つだけ渡した場合，"<prefix>_vehicle0.csv" /
       "<prefix>_vehicle1.csv" という命名規則から，同じディレクトリにある
       もう片方のファイルを自動的に推定して両方を読み込む。
       （exp3_monitor.py は自車のログファイル名だけを知っていればよい）
       python analyze_exp3.py 20260827_135529_vehicle0.csv

  2個: 両方のファイル名を明示的に渡す。
       python analyze_exp3.py 20260827_135529_vehicle0.csv 20260827_135529_vehicle1.csv

  ディレクトリ名を含まないファイル名を渡した場合は LOG_DIR_DEFAULT (log/) から
  読み込む。ディレクトリ込みのパスを渡した場合はそのパスをそのまま使用する。

【出力】
  output/<ファイル名の時間部分>/figure/*.png  ... 各解析グラフ
  output/<ファイル名の時間部分>/report.md      ... 解析結果をまとめたレポート

  「ファイル名の時間部分」は "<prefix>_vehicle0.csv" の <prefix> 部分
  （例: "20260827_135529_vehicle0.csv" -> "20260827_135529"）。

【解析内容】
  1. 速度プロファイル（実速度 vs 目標速度）
  2. 追従誤差（実速度 - 目標速度）
  3. 現在位置（速度の時間積分による推定走行距離）
  4. 交差点通過予測（EnterTime/ExitTime）の収束
  5. 実際の交差点占有区間のタイムライン

必要な想定CSVカラム:
  Time_us, Speed_cm_s, Target_speed_cm_s, Line_count,
  EnterTime_us, ExitTime_us
"""

import os
import sys

import numpy as np
import pandas as pd
from scipy.integrate import cumulative_trapezoid
import matplotlib
matplotlib.use("Agg")  # 非GUI環境でも画像出力できるようにする
import matplotlib.pyplot as plt

# 実行環境によっては日本語フォントが無く文字化けするため，
# 画像(figure/*.png)内のテキストは英語表記のみとし，日本語は使用しない。
# また，グラフタイトルは画像に含めず，report.md 側の見出し・説明文で示す。


# ============================================================
# 定数
#
# ディレクトリ構成の前提:
#   tool/  ... 本スクリプト（本スクリプトの置き場所）
#   log/   ... 走行データ(CSV)一式
#   output/ ... 解析結果の出力先
# を想定し，本スクリプトの置き場所(tool/)からの相対パスで解決する。
# ============================================================
_SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
LOG_DIR_DEFAULT = os.path.normpath(os.path.join(_SCRIPT_DIR, "..", "log"))
OUTPUT_ROOT_DEFAULT = os.path.normpath(os.path.join(_SCRIPT_DIR, "..", "output"))

# 実行時引数が無い場合に使用するデフォルトのファイル名
# （log/ からの相対名。各自の実験で得られたログファイル名に書き換えて使うこと）
DEFAULT_FILES = (
    "20260827_135529_vehicle0.csv",
    "20260827_135529_vehicle1.csv",
)

FIGURE_DIR_NAME = "figure"
REPORT_FILENAME = "report.md"

VEHICLE_LABELS = ("vehicle0", "vehicle1")
VEHICLE_COLORS = ("tab:blue", "tab:orange")

VEHICLE_SUFFIXES = {
    VEHICLE_LABELS[0]: "_vehicle0.csv",
    VEHICLE_LABELS[1]: "_vehicle1.csv",
}


# ------------------------------------------------------------
# ファイルパスの解決
# ------------------------------------------------------------
def resolve_csv_path(filename):
    """ディレクトリを含まないファイル名は LOG_DIR_DEFAULT から読み込む。"""
    if os.path.dirname(filename) != "":
        return filename
    return os.path.join(LOG_DIR_DEFAULT, filename)


def infer_partner_path(csv_path):
    """'<prefix>_vehicleN.csv' の命名規則から，もう片方のファイルパスを推定する。"""
    dirname = os.path.dirname(csv_path)
    base = os.path.basename(csv_path)
    if base.endswith(VEHICLE_SUFFIXES[VEHICLE_LABELS[0]]):
        prefix = base[: -len(VEHICLE_SUFFIXES[VEHICLE_LABELS[0]])]
        partner_base = prefix + VEHICLE_SUFFIXES[VEHICLE_LABELS[1]]
    elif base.endswith(VEHICLE_SUFFIXES[VEHICLE_LABELS[1]]):
        prefix = base[: -len(VEHICLE_SUFFIXES[VEHICLE_LABELS[1]])]
        partner_base = prefix + VEHICLE_SUFFIXES[VEHICLE_LABELS[0]]
    else:
        raise ValueError(
            f"ファイル名からペアとなるファイルを推定できません: '{base}'\n"
            f"'<prefix>{VEHICLE_SUFFIXES[VEHICLE_LABELS[0]]}' / "
            f"'<prefix>{VEHICLE_SUFFIXES[VEHICLE_LABELS[1]]}' の命名規則に"
            "従うファイル名を指定するか，2つのファイル名を両方とも指定してください。"
        )
    return os.path.join(dirname, partner_base)


def resolve_pair(arg_files):
    """
    実行時引数(0個/1個/2個のファイル名)から，vehicle0用・vehicle1用の
    CSVパスを決定する。
    """
    if len(arg_files) == 0:
        raw_paths = [resolve_csv_path(f) for f in DEFAULT_FILES]
    elif len(arg_files) == 1:
        p0 = resolve_csv_path(arg_files[0])
        p1 = infer_partner_path(p0)
        raw_paths = [p0, p1]
    elif len(arg_files) == 2:
        raw_paths = [resolve_csv_path(arg_files[0]), resolve_csv_path(arg_files[1])]
    else:
        raise SystemExit(
            "引数はファイル名のみ，0個・1個・2個のいずれかで指定してください。"
        )

    csv_v0 = next(
        (p for p in raw_paths if os.path.basename(p).endswith(
            VEHICLE_SUFFIXES[VEHICLE_LABELS[0]])), None)
    csv_v1 = next(
        (p for p in raw_paths if os.path.basename(p).endswith(
            VEHICLE_SUFFIXES[VEHICLE_LABELS[1]])), None)

    if csv_v0 is None or csv_v1 is None:
        # 命名規則に合わない場合は，渡された順番をそのまま vehicle0/vehicle1 とみなす
        csv_v0, csv_v1 = raw_paths[0], raw_paths[1]

    return csv_v0, csv_v1


def extract_run_id(csv_v0_path):
    """出力フォルダ名に使う「ファイル名の時間部分」を抽出する。"""
    base = os.path.basename(csv_v0_path)
    suffix = VEHICLE_SUFFIXES[VEHICLE_LABELS[0]]
    if base.endswith(suffix):
        return base[: -len(suffix)]
    return os.path.splitext(base)[0]


# ------------------------------------------------------------
# データ読み込み・前処理
# ------------------------------------------------------------
def load_vehicle_csv(path, label):
    df = pd.read_csv(path)
    required_cols = {
        "Time_us", "Speed_cm_s", "Target_speed_cm_s",
        "Line_count", "EnterTime_us", "ExitTime_us",
    }
    missing = required_cols - set(df.columns)
    if missing:
        raise ValueError(f"{path} に必要な列がありません: {missing}")

    df = df.sort_values("Time_us").reset_index(drop=True)
    df["Time_ms"] = df["Time_us"] / 1000.0
    df["Time_s"] = df["Time_us"] / 1_000_000.0
    df["TrackingError_cm_s"] = df["Speed_cm_s"] - df["Target_speed_cm_s"]
    df["AbsEnter_us"] = df["Time_us"] + df["EnterTime_us"]
    df["AbsExit_us"] = df["Time_us"] + df["ExitTime_us"]

    # 現在位置: 速度(cm/s)を時間(s)で積分した推定走行距離[cm]
    df["Position_cm"] = cumulative_trapezoid(
        df["Speed_cm_s"].to_numpy(), df["Time_s"].to_numpy(), initial=0.0)

    df["label"] = label
    return df


def get_prediction_rows(df):
    return df.dropna(subset=["EnterTime_us", "ExitTime_us"]).copy()


def estimate_actual_crossing(pred_rows):
    """
    実際の交差点通過区間 [enter, exit] (絶対us) を推定する。
    EnterTime_us <= 0 になった行（＝「今まさに進入」）があれば実測値として使用し，
    無ければ最後に記録された予測値で代用する（is_actual=Falseで区別）。
    """
    entered = pred_rows[pred_rows["EnterTime_us"] <= 0]
    if len(entered) > 0:
        enter_us = entered["Time_us"].min()
        exit_us = entered.loc[entered["Time_us"].idxmax(), "AbsExit_us"]
        enter_row = entered.loc[entered["Time_us"].idxmin()]
        return {
            "enter_us": enter_us, "exit_us": exit_us, "is_actual": True,
            "enter_position_cm": enter_row["Position_cm"],
        }

    if len(pred_rows) == 0:
        return {"enter_us": None, "exit_us": None, "is_actual": False,
                "enter_position_cm": None}

    last = pred_rows.loc[pred_rows["Time_us"].idxmax()]
    return {
        "enter_us": last["AbsEnter_us"], "exit_us": last["AbsExit_us"],
        "is_actual": False, "enter_position_cm": None,
    }


# ------------------------------------------------------------
# 1. 速度プロファイル（実速度 vs 目標速度）
# ------------------------------------------------------------
def plot_speed_profile(v0, v1, fig_dir):
    fig, ax = plt.subplots(figsize=(8, 4.5))

    ax.plot(v0["Time_ms"], v0["Speed_cm_s"], color=VEHICLE_COLORS[0],
            linewidth=1.5, label="vehicle0 actual")
    ax.plot(v0["Time_ms"], v0["Target_speed_cm_s"], color=VEHICLE_COLORS[0],
            linewidth=1.0, linestyle="--", alpha=0.6, label="vehicle0 target")
    ax.plot(v1["Time_ms"], v1["Speed_cm_s"], color=VEHICLE_COLORS[1],
            linewidth=1.5, label="vehicle1 actual")
    ax.plot(v1["Time_ms"], v1["Target_speed_cm_s"], color=VEHICLE_COLORS[1],
            linewidth=1.0, linestyle="--", alpha=0.6, label="vehicle1 target")

    ax.set_xlabel("Time [ms]")
    ax.set_ylabel("Speed [cm/s]")
    ax.legend(fontsize=8, ncol=2)
    ax.grid(alpha=0.3)
    fig.tight_layout()

    fname = "01_speed_profile.png"
    fig.savefig(os.path.join(fig_dir, fname), dpi=150)
    plt.close(fig)
    return fname


# ------------------------------------------------------------
# 2. 追従誤差（実速度 - 目標速度）
# ------------------------------------------------------------
def plot_tracking_error(v0, v1, fig_dir):
    fig, ax = plt.subplots(figsize=(8, 4))

    ax.plot(v0["Time_ms"], v0["TrackingError_cm_s"], color=VEHICLE_COLORS[0],
            linewidth=1.2, label="vehicle0")
    ax.plot(v1["Time_ms"], v1["TrackingError_cm_s"], color=VEHICLE_COLORS[1],
            linewidth=1.2, label="vehicle1")
    ax.axhline(0, color="black", linewidth=0.8)

    ax.set_xlabel("Time [ms]")
    ax.set_ylabel("Speed - Target [cm/s]")
    ax.legend(fontsize=8)
    ax.grid(alpha=0.3)
    fig.tight_layout()

    fname = "02_tracking_error.png"
    fig.savefig(os.path.join(fig_dir, fname), dpi=150)
    plt.close(fig)
    return fname


# ------------------------------------------------------------
# 3. 現在位置（速度の時間積分による推定走行距離）
# ------------------------------------------------------------
def plot_position(v0, v1, crossing0, crossing1, fig_dir):
    fig, ax = plt.subplots(figsize=(8, 4.5))

    ax.plot(v0["Time_ms"], v0["Position_cm"], color=VEHICLE_COLORS[0],
            linewidth=1.5, label="vehicle0")
    ax.plot(v1["Time_ms"], v1["Position_cm"], color=VEHICLE_COLORS[1],
            linewidth=1.5, label="vehicle1")

    for crossing, color, label in zip(
            (crossing0, crossing1), VEHICLE_COLORS, VEHICLE_LABELS):
        if crossing["is_actual"]:
            t_ms = crossing["enter_us"] / 1000.0
            pos = crossing["enter_position_cm"]
            ax.scatter([t_ms], [pos], color=color, s=40, zorder=5,
                       marker="o", edgecolor="black")
            ax.annotate(f"{label} enter\n({pos:.0f} cm)",
                        (t_ms, pos), textcoords="offset points",
                        xytext=(6, 6), fontsize=7, color=color)

    ax.set_xlabel("Time [ms]")
    ax.set_ylabel("Position [cm]")
    ax.legend(fontsize=8)
    ax.grid(alpha=0.3)
    fig.tight_layout()

    fname = "03_position.png"
    fig.savefig(os.path.join(fig_dir, fname), dpi=150)
    plt.close(fig)
    return fname


# ------------------------------------------------------------
# 4. 交差点通過予測（EnterTime/ExitTime）の収束
# ------------------------------------------------------------
def plot_prediction_convergence(pred0, pred1, crossing0, crossing1, fig_dir):
    fig, ax = plt.subplots(figsize=(8, 4.5))

    ax.plot(pred0["Time_ms"], pred0["AbsEnter_us"] / 1000.0,
            color=VEHICLE_COLORS[0], marker="o", markersize=3, linewidth=1,
            label="vehicle0 predicted enter")
    ax.plot(pred0["Time_ms"], pred0["AbsExit_us"] / 1000.0,
            color=VEHICLE_COLORS[0], marker="o", markersize=3, linewidth=1,
            linestyle="--", alpha=0.6, label="vehicle0 predicted exit")
    ax.plot(pred1["Time_ms"], pred1["AbsEnter_us"] / 1000.0,
            color=VEHICLE_COLORS[1], marker="o", markersize=3, linewidth=1,
            label="vehicle1 predicted enter")
    ax.plot(pred1["Time_ms"], pred1["AbsExit_us"] / 1000.0,
            color=VEHICLE_COLORS[1], marker="o", markersize=3, linewidth=1,
            linestyle="--", alpha=0.6, label="vehicle1 predicted exit")

    for crossing, color in zip((crossing0, crossing1), VEHICLE_COLORS):
        if crossing["enter_us"] is not None:
            ax.axhline(crossing["enter_us"] / 1000.0, color=color,
                       linewidth=0.8, alpha=0.4)

    ax.set_xlabel("Time [ms] (timestamp of each log row)")
    ax.set_ylabel("Predicted absolute time [ms]")
    ax.legend(fontsize=8, ncol=2)
    ax.grid(alpha=0.3)
    fig.tight_layout()

    fname = "04_prediction_convergence.png"
    fig.savefig(os.path.join(fig_dir, fname), dpi=150)
    plt.close(fig)
    return fname


# ------------------------------------------------------------
# 5. 実際の交差点占有区間のタイムライン
# ------------------------------------------------------------
def plot_timeline_gantt(crossing0, crossing1, fig_dir):
    fig, ax = plt.subplots(figsize=(8, 2.6))

    rows = [
        (VEHICLE_LABELS[0], crossing0, VEHICLE_COLORS[0], 1),
        (VEHICLE_LABELS[1], crossing1, VEHICLE_COLORS[1], 0),
    ]
    for label, crossing, color, y in rows:
        if crossing["enter_us"] is None:
            continue
        start_ms = crossing["enter_us"] / 1000.0
        dur_ms = (crossing["exit_us"] - crossing["enter_us"]) / 1000.0
        alpha = 1.0 if crossing["is_actual"] else 0.4
        ax.broken_barh([(start_ms, dur_ms)], (y - 0.35, 0.7),
                        facecolors=color, alpha=alpha)
        note = "" if crossing["is_actual"] else " (not entered, last prediction)"
        ax.text(start_ms, y, f" {label}{note}", va="center",
                fontsize=8, color="black")

    ax.set_yticks([0, 1])
    ax.set_yticklabels(VEHICLE_LABELS[::-1])
    ax.set_xlabel("Time [ms]")
    ax.grid(alpha=0.3, axis="x")
    fig.tight_layout()

    fname = "05_timeline_gantt.png"
    fig.savefig(os.path.join(fig_dir, fname), dpi=150)
    plt.close(fig)
    return fname


# ------------------------------------------------------------
# レポート本文の組み立て
# ------------------------------------------------------------
def crossing_description(label, crossing):
    if crossing["enter_us"] is None:
        return f"- {label}: 交差点通過に関する予測データがありませんでした。"
    tag = "実測" if crossing["is_actual"] else "未進入（最終予測値を使用）"
    desc = (f"- {label}: 進入 {crossing['enter_us']/1000:.1f} ms, "
            f"退出 {crossing['exit_us']/1000:.1f} ms （{tag}）")
    if crossing["is_actual"] and crossing["enter_position_cm"] is not None:
        desc += f"，進入時点の推定走行距離 {crossing['enter_position_cm']:.0f} cm"
    return desc


def build_report_markdown(csv_v0, csv_v1, images, crossing0, crossing1):
    lines = []
    lines.append("# 実験3 走行データ解析レポート")
    lines.append("")
    lines.append("## 対象データ")
    lines.append("")
    lines.append(f"- vehicle0: `{csv_v0}`")
    lines.append(f"- vehicle1: `{csv_v1}`")
    lines.append("")

    lines.append("## 1. 速度プロファイル（実速度 vs 目標速度）")
    lines.append("")
    lines.append(f"![speed profile]({FIGURE_DIR_NAME}/{images['speed_profile']})")
    lines.append("")
    lines.append(
        "2台の実速度と目標速度を同一時間軸で重ねたグラフ。目標速度が"
        "変化した箇所（衝突回避判断のタイミング）と，そこから実速度が"
        "追従していく様子を確認する。"
    )
    lines.append("")

    lines.append("## 2. 追従誤差（実速度 - 目標速度）")
    lines.append("")
    lines.append(f"![tracking error]({FIGURE_DIR_NAME}/{images['tracking_error']})")
    lines.append("")
    lines.append(
        "0からの乖離が大きい区間は目標速度への追従が遅れている，"
        "または制御が振動していることを示す。"
    )
    lines.append("")

    lines.append("## 3. 現在位置（推定走行距離）")
    lines.append("")
    lines.append(f"![position]({FIGURE_DIR_NAME}/{images['position']})")
    lines.append("")
    lines.append(
        "`Speed_cm_s` を時間積分して求めた推定走行距離。実際に交差点へ"
        "進入した時刻における推定走行距離を丸印で示している。両車の"
        "スタート位置から交差点までの物理的な距離が同程度であれば，"
        "この進入時点の推定走行距離も両車で近い値になるはずであり，"
        "速度積分による位置推定の妥当性を確認する目安になる。"
    )
    lines.append("")
    lines.append(crossing_description(VEHICLE_LABELS[0], crossing0))
    lines.append(crossing_description(VEHICLE_LABELS[1], crossing1))
    lines.append("")

    lines.append("## 4. 交差点通過予測の収束")
    lines.append("")
    lines.append(f"![prediction convergence]({FIGURE_DIR_NAME}/{images['convergence']})")
    lines.append("")
    lines.append(
        "各時刻において記録された「予測進入時刻」「予測退出時刻」の推移。"
        "値が時間とともに一定値へ収束していれば，予測アルゴリズムが安定"
        "して機能していると判断できる。振動しながら収束する場合は，速度"
        "推定のノイズや制御の遅れが予測精度に影響している可能性がある。"
    )
    lines.append("")

    lines.append("## 5. 実際の交差点占有区間のタイムライン")
    lines.append("")
    lines.append(f"![timeline gantt]({FIGURE_DIR_NAME}/{images['gantt']})")
    lines.append("")
    lines.append(crossing_description(VEHICLE_LABELS[0], crossing0))
    lines.append(crossing_description(VEHICLE_LABELS[1], crossing1))
    lines.append("")
    if crossing0["enter_us"] is not None and crossing1["enter_us"] is not None:
        if crossing0["enter_us"] <= crossing1["enter_us"]:
            first, second = crossing0, crossing1
            first_label, second_label = VEHICLE_LABELS
        else:
            first, second = crossing1, crossing0
            second_label, first_label = VEHICLE_LABELS
        gap_ms = (second["enter_us"] - first["exit_us"]) / 1000.0
        verdict = "重複なし（安全）" if gap_ms >= 0 else "**重複あり（衝突リスク）**"
        lines.append(
            f"- {first_label} が退出してから {second_label} が進入するまでの"
            f"間隔: **{gap_ms:.1f} ms** → 判定: {verdict}"
        )
        lines.append("")

    return "\n".join(lines)


# ------------------------------------------------------------
# メイン解析処理
# ------------------------------------------------------------
def run_analysis(arg_files):
    csv_v0, csv_v1 = resolve_pair(arg_files)

    for path, label in [(csv_v0, VEHICLE_LABELS[0]), (csv_v1, VEHICLE_LABELS[1])]:
        if not os.path.exists(path):
            raise FileNotFoundError(f"{label} のCSVが見つかりません: {path}")

    run_id = extract_run_id(csv_v0)
    run_outdir = os.path.join(OUTPUT_ROOT_DEFAULT, run_id)
    fig_dir = os.path.join(run_outdir, FIGURE_DIR_NAME)
    os.makedirs(fig_dir, exist_ok=True)

    v0 = load_vehicle_csv(csv_v0, VEHICLE_LABELS[0])
    v1 = load_vehicle_csv(csv_v1, VEHICLE_LABELS[1])

    pred0 = get_prediction_rows(v0)
    pred1 = get_prediction_rows(v1)
    crossing0 = estimate_actual_crossing(pred0)
    crossing1 = estimate_actual_crossing(pred1)

    images = {}
    images["speed_profile"] = plot_speed_profile(v0, v1, fig_dir)
    images["tracking_error"] = plot_tracking_error(v0, v1, fig_dir)
    images["position"] = plot_position(v0, v1, crossing0, crossing1, fig_dir)
    images["convergence"] = plot_prediction_convergence(
        pred0, pred1, crossing0, crossing1, fig_dir)
    images["gantt"] = plot_timeline_gantt(crossing0, crossing1, fig_dir)

    report_md = build_report_markdown(csv_v0, csv_v1, images, crossing0, crossing1)

    report_path = os.path.join(run_outdir, REPORT_FILENAME)
    with open(report_path, "w", encoding="utf-8") as f:
        f.write(report_md)

    print(f"[analyze_exp3] レポートを生成しました: {os.path.abspath(report_path)}")
    return os.path.abspath(report_path)


# ------------------------------------------------------------
# CLIエントリポイント（受け取るのはファイル名のみ。他のオプションは持たない）
# ------------------------------------------------------------
def main():
    arg_files = sys.argv[1:]
    run_analysis(arg_files)


if __name__ == "__main__":
    main()