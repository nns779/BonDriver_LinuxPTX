# BonDriver_LinuxPTX

各種ISDB-T/Sチューナー用のいわゆるchardev版ドライバによって生成されるチューナーデバイスファイルを、BonDriverとして使用するためのプログラムです。  
[BonDriverProxy_Linux](https://github.com/u-n-k-n-o-w-n/BonDriverProxy_Linux)や[recbond](https://github.com/dogeel/recbond)などの、Linux版BonDriverに対応しているソフトウェアと共に使用できます。  
作成にあたっては、BonDriverProxy_Linuxやそれに同梱のBonDriver_LinuxPTを参考にさせて頂きました。

## 特長

BonDriverProxy_Linuxに同梱されているBonDriver_LinuxPTとは異なり、PLEX PX-MLT5PEやe-better DTV02A-1T1S-Uといった、チューナーデバイスファイル1つでISDB-TとISDB-Sを切り替えて使用できるチューナーにも対応しています。

## 使い方

### ビルド

g++, make, 各種ヘッダファイルがインストールされている必要があります。

	$ make

ビルドが完了すると、`BonDriver_LinuxPTX.so`という名前のファイルが生成されます。

### コピー

`BonDriver_LinuxPTX.so`と`BonDriver_LinuxPTX.ini`を、BonDriverを読み込ませたいソフトウェアの指定するディレクトリにコピーしてください。  
複数のチューナーを同時に使用したい場合は、上記2つのファイルを使用したいチューナーの数だけコピーしてください。  
コピー後に、iniファイルのデバイス名・デバイスファイル名やチャンネル空間設定を適切なものに書き換えてください。
