# CollisionAvoidance

## 必要なソフト

- VSCode
- PlatformIO IED
- C/C++
- C/C++ Extension Pack
- Python
- Pylance
- Python Debugger
- Python Enviroments

- Python 3.11以上

## 初回セットアップ

Linux

```bash
sudo chmod +x setup.sh 
./setup.sh
source .venv/bin/activate
```
Windows

```bash
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
.\setup.ps1
.\.venv\Scripts\Activate.ps1
```



# PlatformIO セットアップ時のトラブルシューティング

## 1. ビルドがうまくいかない場合

ビルドに失敗する場合は、PlatformIO の `Espressif 32` を一度アンインストールしてから、再度ビルドを行う。

### GUIからアンインストール

VS Code 左側の **PlatformIO アイコン（アリのマーク）** を開き、

`QUICK ACCESS` → `PIO Home` → `Platforms`

から **Espressif 32** を探してアンインストールする。

その後、再度ビルドを実行する。

### GUIから削除できない場合

PlatformIO のファイルを手動で削除する。

まず、PlatformIO Core CLI を開く。

VS Code 上部の検索ボックスで、

```text
>PlatformIO Open PlatformIO Core CLI
```

と入力して実行する。

その後、以下のコマンドで `.platformio` の場所を確認する。

```powershell
pio system info
```

表示された情報から `.platformio` の場所を確認する。

`.platformio` が見つかったら、以下のフォルダを削除する。

```text
.platformio\packages\toolchain-riscv32-esp
```

```text
.platformio\platforms\espressif32
```

削除後、再度 PlatformIO でビルドを実行する。

必要に応じて、以下のコマンドで `espressif32` を再インストールする。

```powershell
pio pkg install --global --platform platformio/espressif32
```

---

## 2. Python がうまく動かない場合

Python 関連のエラーが発生した場合は、プロジェクト内の `.venv` を削除してから、セットアップをもう一度実行する。

### 手順

1. プロジェクトフォルダを開く
2. `.venv` フォルダを削除する
3. セットアップをもう一度実行する
4. セットアップ完了後、プログラムを再度実行する

---

## 3. `ModuleNotFoundError: No module named 'serial'` が出る場合

以下のようなエラーが表示された場合、

```text
ModuleNotFoundError: No module named 'serial'
```

`pyserial` がインストールされていない可能性がある。

### Windows（PowerShell）の場合

PowerShell で以下を実行する。

```powershell
python -m pip install pyserial
```

### Linux（bash）の場合

bash で以下を実行する。

```bash
python -m pip install pyserial
```

インストール後、もう一度プログラムを実行する。

---

# PlatformIO Core CLI

## CLI の開き方

VS Code 上部の検索ボックスで、

```text
>PlatformIO Open PlatformIO Core CLI
```

と入力して実行する。

PlatformIO のコマンドを実行する場合は、この CLI を使用する。

---

## よく使用するコマンド

### 1. PlatformIO のシステム情報を確認する

```powershell
pio system info
```

PlatformIO の環境情報を確認する。

特に、`.platformio` の保存場所を確認するときに使用する。

---

### 2. PlatformIO のパッケージを確認する

```powershell
pio pkg list
```

現在インストールされている PlatformIO のパッケージやプラットフォームを確認する。

PlatformIO の環境に何がインストールされているか調査するときに使用する。

---

### 3. Espressif 32 を再インストールする

```powershell
pio pkg install --global --platform platformio/espressif32
```

PlatformIO の `Espressif 32` プラットフォームを再インストールする。

`Espressif 32` に関連するファイルが壊れている場合などに使用する。

---

# トラブルシューティングの基本手順

PlatformIO のビルドに問題が発生した場合は、以下の順番で確認する。

1. PlatformIO の `Espressif 32` をアンインストールする
2. 再度ビルドする
3. 改善しない場合は `.platformio` 内の関連フォルダを確認する
4. 必要に応じて以下を手動削除する

```text
.platformio\packages\toolchain-riscv32-esp
.platformio\platforms\espressif32
```

5. 必要に応じて `Espressif 32` を再インストールする

```powershell
pio pkg install --global --platform platformio/espressif32
```

6. Python 関連の問題の場合は `.venv` を削除してセットアップをやり直す
7. `serial` が見つからない場合は `pyserial` をインストールする

```powershell
python -m pip install pyserial
```