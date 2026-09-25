#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
docs/_common.py

docs/build.py と docs/distribute.py で共有するロジック
（プロジェクトルート/docs/testの位置、exclude_*.txtの読み込み、
ファイル探索、test/による差し替えの解決）をまとめたモジュール。

コードの二重管理を避けるため、探索・差し替えの仕組み自体はここに一本化する。
一方で「何を除外するか」は build.py と distribute.py で目的が異なる
（前者は実験書への掲載可否、後者は配布物への収録可否）ため、
除外パターンのファイルはあえて分離している
（EXCLUDE_FILE_BUILD / EXCLUDE_FILE_DISTRIBUTE）。
このファイル自体は docs/ 以下にあるため、常に付録・配布対象からは除外される。
"""

from __future__ import annotations

import fnmatch
import os
from pathlib import Path

# ============================================================
# パスの基準点
# ============================================================

DOCS_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = DOCS_DIR.parent

TEST_DIR = PROJECT_ROOT / "test"

# build.py（実験書への付録埋め込み）用の除外リスト
EXCLUDE_FILE_BUILD = DOCS_DIR / "exclude_build.txt"
# distribute.py（配布用フォルダ作成）用の除外リスト
# build.py とは目的が異なる（配布したくない資料等を除外する）ため、
# あえて別ファイルとして独立させている。
EXCLUDE_FILE_DISTRIBUTE = DOCS_DIR / "exclude_distribute.txt"

# 後方互換のため、旧名 EXCLUDE_FILE は build 用の別名として残す
EXCLUDE_FILE = EXCLUDE_FILE_BUILD

EXPERIMENT_MD = DOCS_DIR / "実験書.md"
OUTPUT_PDF = DOCS_DIR / "実験書.pdf"

# 常に除外するトップレベル名（深さを問わず、この名前のディレクトリは除外する）
ALWAYS_EXCLUDED_DIR_NAMES = {"docs", "test"}


# ============================================================
# exclude.txt の読み込み
# ============================================================

def load_exclude_patterns(path: Path = EXCLUDE_FILE_BUILD) -> list[str]:
    """exclude.txt を読み込み、コメント・空行を除いたパターンのリストを返す。

    仕様:
      - '#' で始まる行はコメント
      - 空行は無視
      - 末尾が '/' のパターンはディレクトリのみに一致
      - '*' 等のワイルドカードは fnmatch 相当で解釈
    """
    if not path.exists():
        return []
    patterns = []
    for raw_line in path.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()
        if not line or line.startswith("#"):
            continue
        patterns.append(line)
    return patterns


def is_excluded(rel_posix_path: str, name: str, is_dir: bool, patterns: list[str]) -> bool:
    """rel_posix_path（プロジェクトルートからの相対パス、'/' 区切り）が
    exclude.txt のいずれかのパターンに一致するかを判定する。
    ファイル名（basename）と相対パスの両方に対して照合する。
    """
    for pat in patterns:
        dir_only = pat.endswith("/")
        pat_clean = pat[:-1] if dir_only else pat
        if dir_only and not is_dir:
            continue
        if fnmatch.fnmatch(name, pat_clean) or fnmatch.fnmatch(rel_posix_path, pat_clean):
            return True
    return False


# ============================================================
# プロジェクト全体の再帰探索
# ============================================================

def walk_project(root: Path = PROJECT_ROOT, exclude_patterns: list[str] | None = None) -> list[str]:
    """root 以下を再帰探索し、付録・配布対象となるファイルの相対パス
    （'/' 区切り、root からの相対）のリストをソート済みで返す。

    - docs/ , test/ は深さを問わず常に除外する
    - シンボリックリンク（ファイル・ディレクトリとも）は辿らない
    - exclude.txt のパターンに一致するものは除外する
    """
    if exclude_patterns is None:
        exclude_patterns = load_exclude_patterns()

    collected: list[str] = []

    for dirpath, dirnames, filenames in os.walk(root, followlinks=False):
        dirpath_p = Path(dirpath)
        rel_dir = dirpath_p.relative_to(root)
        rel_dir_str = "" if str(rel_dir) == "." else str(rel_dir).replace(os.sep, "/")

        # ディレクトリの枝刈り（in-place で dirnames を書き換えると os.walk に反映される）
        kept_dirs = []
        for d in dirnames:
            full = dirpath_p / d
            rel = f"{rel_dir_str}/{d}" if rel_dir_str else d

            if d in ALWAYS_EXCLUDED_DIR_NAMES:
                continue
            if full.is_symlink():
                continue
            if is_excluded(rel, d, True, exclude_patterns):
                continue
            kept_dirs.append(d)
        dirnames[:] = sorted(kept_dirs)

        for f in filenames:
            full = dirpath_p / f
            rel = f"{rel_dir_str}/{f}" if rel_dir_str else f

            if full.is_symlink():
                continue
            if is_excluded(rel, f, False, exclude_patterns):
                continue
            collected.append(rel)

    # 仕様: 「ファイルはプロジェクトルートからの相対パスを基準にソートする」
    return sorted(collected)


# ============================================================
# test/ による差し替えの解決
# ============================================================

def resolve_source_path(rel_path: str) -> Path:
    """test/ による差し替えを解決する。
    test/<rel_path> が存在すればそちらを、無ければ元のファイルを返す。
    （test/ 以下はプロジェクトルートからの相対パス構造をそのままミラーする想定）
    """
    test_candidate = TEST_DIR / rel_path
    if test_candidate.is_file():
        return test_candidate
    return PROJECT_ROOT / rel_path