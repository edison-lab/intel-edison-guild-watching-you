詳細：http://edison-lab.jp/showcase/theguild/
Author: http://theguild.jp/



プロジェクト材料
•	インテル® Edison キット For Arduino < http://edison-lab.jp/edison-arduino/ >
•	6個の超音波距離センサ（型番 HC-SR04）
•	Mac Mini （Open Frameworks, Open Source Control (OSC), Unity とLive 2Dを使用）
•	縦型モニター（60インチ）


・システム構成
【Edison + センサ】→（シリアル通信）→【oF（Mac mini）】→（OSC）→【Unity + Live2D（Mac mini）】

・【Edison + センサ】
センサーとして6個の超音波距離センサを使用する（型番 HC-SR04）。
6個のセンサーからの情報をEdison内で集約し、床面座標系（3D）で人物を検出する。
3D空間上での検出座標をモニタ座標系（2D）に変換し、シリアル通信で外部に出力する。
座標系は axis.png を参照。
ソースコードは i_am_watching_you.ino を参照。

・【oF（Mac mini）】
openFrameworksでEdisonからのシリアル通信を受け取り、
OCSプロトコルに変換してMac mini内のローカルネットワーク上に出力する。

・【Unity + Live2D（Mac mini）】
oFからのOCS信号をUnityで受け取り、
送られてきたモニタ座標系のxy座標を見るようにアニメーションを駆動する。
