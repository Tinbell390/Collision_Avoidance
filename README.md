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

ビルドがうまく行かないなら，左列のPIOアイコン(アリマーク)の`QUICK ACCESS/PIO Home/Platform`にある`Espressif 32`をアンインストールする．
(これがうまく行かなければ，`.platformio`をCLIで探して移動し，`\.platformio\packages\toolchain-riscv32-esp`と`\.platformio\platforms\espressif32`を手動で削除する)
その後，ビルドを行う．


pythonがうまく動かない場合,`.venv`を削除してセットアップをもう一度行う．その後，もう一度実行する．

`ModuleNotFoundError: No module named 'serial'`とでたら，powershell(windows) or bash(Linux)で`python -m pip install pyserial`を実行する

platformIOのCLIの開き方
- 上の検索ボックスで`>platformIO Open PlatformIO Core CLI`を入力
    - `pio system info`
        - `.platformio`を探すために使用
    - `pio pkg list`
        - `platformio`の環境を調査
    - `pio pkg install --global --platform platformio/espressif32`
        - espressif32を再インストール

