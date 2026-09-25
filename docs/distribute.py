#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
docs/distribute.py

学生配布用フォルダを生成する。

- プロジェクトルートと同じ階層（1つ上のディレクトリ）に
  "2026_higlab_設計製作_配布用" というフォルダを新規作成する。
  既に存在する場合は、一度削除してから作り直す（＝1から生成）。
- ファイル探索・test/による差し替えの仕組みは docs/build.py と共通で、
  docs/_common.py のロジックをそのまま使う:
    - docs/ と test/ は除外
    - 収録対象ファイル X について test/X が存在すればそちらを使う
      （穴埋め版への差し替え）
  ただし「何を除外するか」は build.py（実験書への掲載可否）とは目的が異なるため、
  除外パターンは docs/exclude_build.txt を共有せず、独立したファイル
  docs/exclude_distribute.txt に列挙する。
  （実験書には載せたくないが配布はしたいファイル／その逆、のどちらも表現できる）
  また、Markdownへの埋め込みではなく実ファイルをそのままコピーするので、
  バイナリファイルやファイルサイズ上限などの制限（build.py側の都合）は適用しない。
- 最後に docs/実験書.pdf を配布用フォルダの直下（サブフォルダなし）にコピーする。
  事前に実験書.md をPDF化（VSCodeのMarkdown PDF拡張など）し、
  docs/実験書.pdf として保存しておく必要がある。

実行方法（プロジェクトルートから）:

    python docs/distribute.py
"""

from __future__ import annotations

import shutil
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import _common as common  # noqa: E402

# 配布用フォルダ名・配置場所（プロジェクトルートと同じ階層 = 兄弟ディレクトリ）
DIST_DIR_NAME = "2026_higlab_設計製作_配布用"
DIST_ROOT = common.PROJECT_ROOT.parent / DIST_DIR_NAME


def recreate_dist_root() -> None:
    if DIST_ROOT.exists():
        print(f"[distribute.py] 既存の {DIST_ROOT} を削除します。")
        shutil.rmtree(DIST_ROOT)
    DIST_ROOT.mkdir(parents=True)
    print(f"[distribute.py] {DIST_ROOT} を作成しました。")


def copy_project_files(rel_paths: list[str]) -> None:
    for rel_path in rel_paths:
        src = common.resolve_source_path(rel_path)
        dst = DIST_ROOT / rel_path
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(src, dst)


def copy_experiment_pdf() -> None:
    pdf_src = common.OUTPUT_PDF
    if not pdf_src.exists():
        print(
            f"[エラー] {pdf_src} が見つかりません。\n"
            "先に実験書.md をPDF化（VSCodeのMarkdown PDF拡張など）し、\n"
            f"{pdf_src} として保存してから再実行してください。",
            file=sys.stderr,
        )
        sys.exit(1)

    pdf_dst = DIST_ROOT / pdf_src.name
    shutil.copy2(pdf_src, pdf_dst)
    print(f"[distribute.py] {pdf_dst} を配置しました。")


def main():
    recreate_dist_root()

    exclude_patterns = common.load_exclude_patterns(common.EXCLUDE_FILE_DISTRIBUTE)
    rel_paths = common.walk_project(common.PROJECT_ROOT, exclude_patterns)
    print(f"[distribute.py] コピー対象ファイル数: {len(rel_paths)}")

    copy_project_files(rel_paths)
    copy_experiment_pdf()

    print(f"[distribute.py] 完了しました: {DIST_ROOT}")


if __name__ == "__main__":
    main()