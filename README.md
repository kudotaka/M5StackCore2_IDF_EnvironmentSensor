# M5StackCore2_IDF_EnvironmentSensor

### M5StackCore2とCO2・温湿度センサで環境測定を試してみる。

### 使用機材
- [M5Stack Core2 IoT開発キット](https://www.switch-science.com/products/6530)
- [M5Stack用SCD40搭載CO2ユニット（温湿度センサ付き）](https://www.switch-science.com/collections/m5stack/products/8496)
- [Grove - SCD30搭載 CO2・温湿度センサ（Arduino用）](https://www.switch-science.com/collections/seeed/products/7000)
- [LGVL(Light nad Versatile Graphics Library)](https://lvgl.io/)

### 動作
- CO2・温湿度を測定する。
- ディスプレイに表示する。
- ディスプレイの表示ON/OFFを中央ボタンの長押しで制御する。
- 00分で音を鳴らす。
- 音のON/OFFを左ボタンの長押しで制御する。
- 定期的に時刻同期を行う。
- 最後の時刻同期時間を右ボタンの長押しで表示する。