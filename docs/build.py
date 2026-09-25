#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
docs/build.py

プロジェクト内の実ファイルから「ファイル構造」と「プログラム（ソースコード）」を
自動生成して docs/実験書.md 内のマーカーに埋め込む。

このスクリプトは実験書.md の書き換えのみを行い、PDF化は行わない。
PDF化はVSCodeのMarkdown PDF拡張など、別の手段で行うことを想定している。

想定される実行方法（プロジェクトルートから）:

    python docs/build.py

使い方:
  1. docs/実験書.md の中に、あらかじめ以下の4行（2組のマーカー）を書いておく。

     <!-- A begin auto generated appendix -->
     <!-- A end auto generated appendix -->

     <!-- B begin auto generated appendix -->
     <!-- B end auto generated appendix -->

     A の組にはファイル構造（図）、B の組にはソースコード（リスト）が
     それぞれ書き込まれる。

  2. python docs/build.py を実行すると、各マーカーの間にあった内容を消去し、
     最新の内容で書き直す。マーカー自体・マーカーの外側の本文は変更しない。

設計方針（詳細は元の設計書を参照）:
  - ファイル探索・test/による差し替えの仕組みは docs/_common.py に一本化されており、
    docs/distribute.py と共有する。ただし除外パターンは docs/exclude_build.txt を
    使用し、distribute.py の docs/exclude_distribute.txt とは独立している。
  - マーカーで囲まれた部分は自動生成物であり、人間が直接編集することは
    想定しない（次回実行で上書きされる）。
  - LaTeXは使用できない環境のため、図・リストのキャプションはすべてHTMLタグ
    （<p style="text-align:center;">等）で中央揃えにしている。
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import _common as common  # noqa: E402

# ============================================================
# 設定
# ============================================================

DOCS_DIR = common.DOCS_DIR
PROJECT_ROOT = common.PROJECT_ROOT
EXCLUDE_FILE = common.EXCLUDE_FILE_BUILD
EXPERIMENT_MD = common.EXPERIMENT_MD

# 実験書.md 内のマーカー（それぞれ実験書.md 内に1組だけ書いておく）
FILE_STRUCTURE_START_MARKER = "<!-- A begin auto generated appendix -->"
FILE_STRUCTURE_END_MARKER = "<!-- A end auto generated appendix -->"
SOURCE_CODE_START_MARKER = "<!-- B begin auto generated appendix -->"
SOURCE_CODE_END_MARKER = "<!-- B end auto generated appendix -->"

# ファイル構造の図キャプション（実験書内の図番号に合わせて変更する）
FIGURE_LABEL = "図5"
FIGURE_CAPTION_TEXT = "プロジェクトのファイル構造"

# プログラムのリストキャプションの接頭辞（"リスト1", "リスト2", ... と連番になる）
LISTING_LABEL_PREFIX = "リスト"

# これより大きいファイルは付録に含めず、警告を出して省略する（バイト単位）
MAX_FILE_SIZE = 2 * 1024 * 1024  # 2MB

# 表示上のルート名（ファイル構造ツリーの一番上に出す名前）
TREE_ROOT_LABEL = PROJECT_ROOT.name + "/"

# 拡張子 -> Markdown コードブロック言語 (= pygments のレクサ名)
EXTENSION_LANG_MAP = {
    ".cpp": "cpp",
    ".cxx": "cpp",
    ".cc": "cpp",
    ".hpp": "cpp",
    ".hxx": "cpp",
    ".c": "c",
    ".h": "c",
    ".py": "python",
    ".sh": "bash",
    ".bash": "bash",
    ".ps1": "powershell",
    ".ini": "ini",
    ".yaml": "yaml",
    ".yml": "yaml",
    ".json": "json",
    ".txt": "text",
    ".md": "text",
    ".js": "javascript",
    ".ts": "typescript",
}


# ============================================================
# ファイル構造の Markdown ツリー生成
# ============================================================

def build_tree_dict(paths: list[str]) -> dict:
    tree: dict = {}
    for p in paths:
        parts = p.split("/")
        node = tree
        for part in parts[:-1]:
            node = node.setdefault(part, {})
        node.setdefault("__files__", []).append(parts[-1])
    return tree


def render_tree(tree: dict) -> str:
    lines = [TREE_ROOT_LABEL]
    lines.extend(_render_tree_level(tree, ""))
    return "\n".join(lines)


def _render_tree_level(node: dict, prefix: str) -> list[str]:
    dirs = sorted(k for k in node.keys() if k != "__files__")
    files = sorted(node.get("__files__", []))
    entries = [(d, True) for d in dirs] + [(f, False) for f in files]
    entries.sort(key=lambda e: e[0].lower())

    lines = []
    for i, (name, is_dir) in enumerate(entries):
        is_last = i == len(entries) - 1
        connector = "└── " if is_last else "├── "
        if is_dir:
            lines.append(f"{prefix}{connector}{name}/")
            extension = "    " if is_last else "│   "
            lines.extend(_render_tree_level(node[name], prefix + extension))
        else:
            lines.append(f"{prefix}{connector}{name}")
    return lines


# ============================================================
# ファイル内容の取得・言語判定・コードフェンス生成
# ============================================================

def detect_language(rel_path: str) -> str:
    ext = Path(rel_path).suffix.lower()
    return EXTENSION_LANG_MAP.get(ext, "text")


def read_file_safely(path: Path) -> tuple[str | None, str | None]:
    """UTF-8 テキストとして読み込む。失敗した場合は (None, エラーメッセージ) を返す。"""
    try:
        size = path.stat().st_size
    except OSError as e:
        return None, f"[読み込みエラー: {e}]"

    if size > MAX_FILE_SIZE:
        return None, f"[このファイルは {size} バイトと大きいため省略しました]"

    try:
        data = path.read_bytes()
        text = data.decode("utf-8")
    except UnicodeDecodeError:
        return None, "[バイナリまたは UTF-8 以外のファイルのため省略しました]"
    except OSError as e:
        return None, f"[読み込みエラー: {e}]"

    # 改行コードを LF に統一
    text = text.replace("\r\n", "\n").replace("\r", "\n")
    return text, None


def make_code_fence(text: str) -> str:
    """本文中に含まれるバッククォート連続数より 1 つ長いフェンスを使う。
    これにより、コード内に ``` が含まれていても Markdown が壊れない。
    """
    max_ticks = 0
    for m in re.finditer(r"`+", text):
        max_ticks = max(max_ticks, len(m.group()))
    fence_len = max(3, max_ticks + 1)
    return "`" * fence_len


def html_center(inner_html: str) -> str:
    """LaTeXが使えない環境向けに、HTMLタグで中央揃えにする。"""
    return f'<p style="text-align: center;">{inner_html}</p>'


# ============================================================
# マーカーA（ファイル構造）に埋め込む内容の生成
# ============================================================

def generate_file_structure_block(rel_paths: list[str]) -> str:
    """コードブロック（ツリー）+ 下に中央揃えのキャプション（図n ...）。"""
    tree = build_tree_dict(rel_paths)
    tree_text = render_tree(tree)

    parts = [
        f"```text\n{tree_text}\n```",
        html_center(f"{FIGURE_LABEL} {FIGURE_CAPTION_TEXT}"),
    ]
    return "\n\n".join(parts) + "\n"


# ============================================================
# マーカーB（プログラム）に埋め込む内容の生成
# ============================================================

def generate_source_code_block(rel_paths: list[str]) -> str:
    """各ファイルについて、上に中央揃えのキャプション（リストn ...）+ コードブロック。"""
    parts = []
    for idx, rel_path in enumerate(rel_paths, start=1):
        source_path = common.resolve_source_path(rel_path)
        text, err = read_file_safely(source_path)

        caption = html_center(f"{LISTING_LABEL_PREFIX}{idx} <code>{rel_path}</code>")

        if err is not None:
            body = f"{err}"
        else:
            lang = detect_language(rel_path)
            fence = make_code_fence(text)
            body = f"{fence}{lang}\n{text}\n{fence}"

        parts.append(f"{caption}\n\n{body}")

    return "\n\n".join(parts) + "\n"


# ============================================================
# 実験書.md 内のマーカー間を書き換える
# ============================================================

def replace_between_markers(text: str, start_marker: str, end_marker: str, new_body: str) -> str:
    """text の中で start_marker と end_marker がそれぞれちょうど1つずつ
    見つかることを確認し、その間を new_body で置き換えた新しいテキストを返す。
    見つからない・複数見つかった場合はエラーを出して終了する。
    """
    start_count = text.count(start_marker)
    end_count = text.count(end_marker)

    if start_count == 0 or end_count == 0:
        print(
            f"[エラー] 実験書.md 内にマーカーが見つかりません。\n"
            f"付録を挿入したい場所に、あらかじめ以下の2行を書いてから再実行してください。\n\n"
            f"{start_marker}\n{end_marker}\n",
            file=sys.stderr,
        )
        sys.exit(1)

    if start_count > 1 or end_count > 1:
        print(
            f"[エラー] マーカーが複数見つかりました。実験書.md 内には\n"
            f"{start_marker}\n{end_marker}\n"
            f"をそれぞれ1つだけ置いてください。",
            file=sys.stderr,
        )
        sys.exit(1)

    start_idx = text.index(start_marker) + len(start_marker)
    end_idx = text.index(end_marker)

    if end_idx < start_idx:
        print(f"[エラー] 終了マーカーが開始マーカーより前にあります（{start_marker}）。", file=sys.stderr)
        sys.exit(1)

    before = text[:start_idx]
    after = text[end_idx:]
    return before + "\n\n" + new_body + "\n" + after


def update_experiment_md(file_structure_body: str, source_code_body: str) -> None:
    if not EXPERIMENT_MD.exists():
        print(f"[エラー] {EXPERIMENT_MD} が見つかりません。", file=sys.stderr)
        sys.exit(1)

    text = EXPERIMENT_MD.read_text(encoding="utf-8")
    text = replace_between_markers(
        text, FILE_STRUCTURE_START_MARKER, FILE_STRUCTURE_END_MARKER, file_structure_body
    )
    text = replace_between_markers(
        text, SOURCE_CODE_START_MARKER, SOURCE_CODE_END_MARKER, source_code_body
    )
    EXPERIMENT_MD.write_text(text, encoding="utf-8")


# ============================================================
# エントリポイント
# ============================================================

def main():
    exclude_patterns = common.load_exclude_patterns(EXCLUDE_FILE)
    rel_paths = common.walk_project(PROJECT_ROOT, exclude_patterns)

    print(f"[build.py] 付録対象ファイル数: {len(rel_paths)}")

    file_structure_body = generate_file_structure_block(rel_paths)
    source_code_body = generate_source_code_block(rel_paths)

    update_experiment_md(file_structure_body, source_code_body)
    print(f"[build.py] {EXPERIMENT_MD} のマーカー間（A: ファイル構造 / B: プログラム）を更新しました。")


if __name__ == "__main__":
    main()