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

ビルドがうまく行かないなら，左列のPIOアイコン(アリマーク)の`QUICK ACCESS/PIO Home/Platform`にある`Espressif 32`をアンインストールする．その後，ビルドを行う．
pythonがうまく動かない場合,`.venv`を削除してセットアップをもう一度行う．その後，もう一度実行する．