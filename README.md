# M5StackCore2_IDF_EnvironmentSensor

### M5StackCore2とCO2・温度・湿度・気圧センサで環境測定を試してみる。

### 使用機材
- [M5Stack Core2 IoT開発キット](https://www.switch-science.com/products/6530)
- [M5Stack Core2 for AWS - ESP32 IoT開発キット](https://www.switch-science.com/products/6784)
- [M5Stack用SCD40搭載CO2ユニット（温湿度センサ付き）](https://www.switch-science.com/products/8496)
- [Grove - SCD30搭載 CO2・温湿度センサ（Arduino用）](https://www.switch-science.com/products/7000)
- [M5Stack用温湿度気圧センサユニット Ver.3（ENV Ⅲ）](https://www.switch-science.com/products/7254)
- [M5Stack用気圧センサユニット（QMP6988）](https://www.switch-science.com/products/8663)
- [M5Stack用環境センサユニット ver.2（ENV II）](https://www.switch-science.com/products/6344)
- [ＢＭＥ６８０使用　温湿度・気圧・ガスセンサモジュールキット](https://akizukidenshi.com/catalog/g/gK-14469/)
- [ＣＯ２センサーモジュール　ＭＨ－Ｚ１９Ｃ](https://akizukidenshi.com/catalog/g/gM-16142/)
- [ＡＤＴ７４１０使用　高精度・高分解能　Ｉ２Ｃ・１６Ｂｉｔ　温度センサモジュール](https://akizukidenshi.com/catalog/g/gM-06675/)
- [LGVL(Light nad Versatile Graphics Library)](https://lvgl.io/)

### 動作
- CO2・温度・湿度・気圧を測定する。
- ディスプレイに表示する。
- ディスプレイの表示ON/OFFを中央ボタンの長押しで制御する。
- 00分で音を鳴らす。
- 音のON/OFFを左ボタンの長押しで制御する。
- 定期的に時刻同期を行う。
- 最後の時刻同期時間を表示する。