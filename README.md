# M5Stamp C3 を PC のフロントパネルヘッダに繋いで、電源を Wi-Fi 越しに操作するやつ

![](https://objects-misskey.mewl.me/files/thumbnail-9093522b-71ac-480c-b3f4-894614191c65.webp)

## 必要なもの

-   基板
-   1 x M5Stamp C3 Mate あるいは M5Stamp C3U Mate
    -   Mate のセットでなく単体の場合は、適当なロープロファイルピンヘッダとピンソケットも用意してください
-   5 x 1608 チップ抵抗 / 100Ω 程度
-   3 x SOT-23 Nch MOSFET / BSS138 など
-   1 x SOD-123 定電流ダイオード or 定電流レギュレータ / 20mA 程度 NSI45020AT1G など
-   1 x L 字ピンヘッダー / 2.54 mm ピッチ 1 列 5 ピン
-   1 x L 字ピンソケット / 2.54 mm ピッチ 2 列 5 ピン

## つくりかた

-   下: GND ベタがないほう
-   表面: 表面実装部品のパターンがある面
-   裏面: LED+/LED-/PWR#/GND/NC の印字がある面

1. 表面に、定電流ダイオードと MOSFET をはんだづけする
2. 表面に、抵抗をはんだづけする。下から 5 つ
3. 裏面に、1x5 のピンヘッダをはんだづけする。LED+/LED-/PWR#/GND/NC の印字が見える方向
4. M5Stamp C3 に付属のピンヘッダを、M5Stamp にはんだづけする
5. 表面に、M5Stamp C3 に付属のピンソケットをはんだづけする。
   あるいは、M5Stamp につけたピンヘッダを直接基板にはんだづけしてもよい
6. 表面に、2x5 のピンソケットをはんだづけする。
   シルクで印がしてあるピンはマザーボード側で欠けてあるピンなので、可能ならソケットからピンを抜いて UV レジンか何かで埋めておくとよい
7. M5Stamp にファームウェアを書きこむ

## ファームウェアの書き込み; 初回

1. VisualStudio Code にプラグイン PlatformIO を追加
2. https://github.com/m-hayabusa/RemotePowerSwitch.git を clone する
3. PlatformIO で 2. のディレクトリを開く。VSCode の左側のバー、なんか触覚が生えてる顔のアイコンから
4. 下のバーの「Default (RemotePowerSwitch)」の部分をクリック、M5Stamp-C3 か M5Stamp-C3U かを選択
5. 下のバーの「→」をクリック、Upload
6. 適当なシリアルモニターで、M5Stamp のポートを開く。Arduino IDE が入ってるならそれがおすすめ。PlatformIO の内蔵シリアルモニターは挙動が怪しいような気がする
7. シリアルモニターでこれらを送って設定:

```
WIFI.SSID:(2.4GHz な Wi-Fi ネットワークの SSID)
WIFI.PASSWORD:(Wi-Fi ネットワークのパスワード)
WEB.HOSTNAME:(mDNS で宣伝するホスト名)
WEB.PASSWORD:(Webページから操作するときに必要なパスワード, 設定しなければパスワードなしで操作できます / 解除する場合は空の文字列を設定)
```

8. "http://" + WEB.HOSTNAME + ".local"で接続できるはずです

## ファームウェアの更新

1. PlatformIO でファームウェアをビルド (下のバーの「✔️」)
2. "http://" + WEB.HOSTNAME + ".local/update" をブラウザで開く
3. Firmware.bin のボタンをクリック、プロジェクトのディレクトリ/.pio/M5Stamp-C3/firmware.bin を選択
4. Upload をクリック
5. アップロード・書き込みが終わると自動で再起動します
