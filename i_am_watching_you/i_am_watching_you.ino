/* -------------------------------------------------- *
 *
 * 設定
 *
 * -------------------------------------------------- */

// シリアルモニタにデバッグログではなく通信用情報を出力する場合はtrue
bool RELEASE_MODE = true;


// --------------------------------------------------
// レイアウト

// フロア全体のサイズ(cm)
float const FLOOR_W = 360; // 幅
float const FLOOR_H = 360; // 奥行

// モニタの座標, サイズ(cm)
float const MONITOR_W = 83;                               // 幅
float const MONITOR_H = 10;                               // 厚み
float const MONITOR_X = (FLOOR_W - MONITOR_W) / 2;        // 床面に対する設置x座標
float const MONITOR_Y = 0;                                // 床面に対する設置y座標
float const MONITOR_CENTER_X = MONITOR_X + MONITOR_W / 2; // 床面に対するモニタの中心点のx座標
float const MONITOR_CENTER_Y = MONITOR_Y;                 // 床面に対するモニタの中心点のy座標

// センサボックス座標, サイズ(cm)
float const SENSOR_W = 140;                      // 幅
float const SENSOR_X = (FLOOR_W - SENSOR_W) / 2; // 床面に対する設置x座標
float const SENSOR_Y = 0;                        // 床面に対する設置y座標


// --------------------------------------------------
// センサ

// センサの検出範囲(cm)
float const DETECTING_DIST_MIN = 15;  // 近距離
float const DETECTING_DIST_MAX = 180; // 遠距離

// 超音波距離センサの検出インターバル(ms)
float const DETECTING_INTERVAL = 20;

// センサが物体を消失してから物体が無くなったと判定するまでのタイムラグ(s)
float const DETECTING_LOST_DELAY = 1;

// センサが検出数0の状態から新たに物体を検出したとき、検出したとみなすまでのタイムラグ(s)
float const DETECTING_NOISE_DELAY = 0;

// 赤外線距離センサの検出範囲(cm)
float const INFRAERD_DIST_MIN = 20;  // 近距離
float const INFRAERD_DIST_MAX = 150; // 遠距離


// --------------------------------------------------
// 座標変換

// 離散的な物体検出座標を滑らかに補完補間するためのイージング係数
float const POSITION_EASING = 0.6;

// 床座標をモニタ座標に変換する角度ベースで算出する場合はtrue
bool const LOOK_AT_X_BY_ANGLE = true;

// モニタ座標のyの値をランダムに設定する頻度(s)
float const LOOK_AT_Y_INTERVAL_MIN = 2; // 最短
float const LOOK_AT_Y_INTERVAL_MAX = 5; // 最長

// モニタ座標のyの値をランダム幅
float const LOOK_AT_Y_RANGE = 0.25;





/* -------------------------------------------------- *
 *
 * センサの基本クラス
 *
 * -------------------------------------------------- */
class Sensor {
public:

    // 初期化
    void setup(int index, float localPositionX, float localPositionY, float globalPositionX, float globalPositionY) {
        this->index = index;
        this->localPositionX = localPositionX;
        this->localPositionY = localPositionY;
        this->globalPositionX = globalPositionX;
        this->globalPositionY = globalPositionY;

        notifyMissing();
        implSetup();
    }

    // 更新
    void update() {
        implUpdate();
    }

    // センサのインデックス
    int index;

    // センサボックスを基準としたセンサの配置座標
    float localPositionX;
    float localPositionY;

    // フロア全体を基準としたセンサの配置座標
    float globalPositionX;
    float globalPositionY;

    // 物体検出中の場合はtrue
    bool isDetecting;

    // 検出中の物体までの距離(cm)
    float distance;

    // 検出中の物体までの距離(遠:0 〜 近:1)
    float intensity;

protected:

    // 検出していないときに派生クラスから呼び出す
    void notifyMissing() {

        // 見検出の場合の値をセットする
        isDetecting = false;
        distance = DETECTING_DIST_MAX;
        intensity = 0;
    }

    // 初期化処理（派生クラスでオーバーライドする）
    virtual void implSetup() {
    }

    // 更新処理（派生クラスでオーバーライドする）
    virtual void implUpdate() {
    }
};





/* -------------------------------------------------- *
 *
 * ひとつの超音波距離センサのクラス
 *
 * -------------------------------------------------- */
class UltrasonicSensor : public Sensor {
public:

    UltrasonicSensor(int triggerPin, int echoPin) {
        this->triggerPin = triggerPin;
        this->echoPin = echoPin;
    }

protected:

    virtual void implSetup() {
        // ボードのピンを初期化する
        pinMode(triggerPin, OUTPUT);
        pinMode(echoPin, INPUT);
        digitalWrite(triggerPin, LOW);
    }

    virtual void implUpdate() {
        // 距離検出用の信号を送信する
        digitalWrite(triggerPin, LOW);
        delayMicroseconds(1);
        digitalWrite(triggerPin, HIGH);
        delayMicroseconds(1);
        digitalWrite(triggerPin, LOW);

        // 距離検出用の信号を受信する
        int duration = pulseIn(echoPin, HIGH);

        // 受信結果から物体までの距離を計算する
        isDetecting = false;
        if (duration > 0) {

            // 送信して受信するまでの時間は往復なので半分にする
            distance = duration / 2;

            // 音速を考慮して時間を距離に変換する 340m/s = 34000cm/s = 0.034cm/us
            distance = distance * 340 * 100 / 1000000;

            // 物体までの距離が設定した検出範囲内の場合のみ検出したと見なす
            if (distance >= DETECTING_DIST_MIN && distance <= DETECTING_DIST_MAX) {
                isDetecting = true;
                intensity = 1 - (distance - DETECTING_DIST_MIN) / (DETECTING_DIST_MAX - DETECTING_DIST_MIN);
            }
        }

       // 検出に失敗した場合
       if (!isDetecting) {
            distance = DETECTING_DIST_MAX;
            intensity = 0;
        }

        // シリアルモニタにデバッグ出力する
        if (!RELEASE_MODE) {
            Serial.print(index);
            Serial.print(" ");
            Serial.println(distance);
        }

        //次のセンサとの干渉を防ぐために少し待つ
        delay(DETECTING_INTERVAL);
    }

private:

    // 送信用ピン番号
    int triggerPin;

    // 受信用ピン番号
    int echoPin;
};





/* -------------------------------------------------- *
 *
 * ひとつの赤外線距離センサのクラス
 *
 * -------------------------------------------------- */
class InfraredSensor : public Sensor {
public:

    InfraredSensor(int outputPin) {
        this->outputPin = outputPin;
    }

protected:

    virtual void implSetup() {
    }

    virtual void implUpdate() {

        // 100回計測して平均値を求める
        int n = 100;
        float value = 0;
        for (int i = 0; i < n; ++i) {
            value += analogRead(outputPin);
        }
        value /= n;
        value = round(value);

        // 電圧の強度を距離に変換する
        // センサ付属の電圧特性はあまり当てにならないので実測した
        distance = value >= 495 ?  15 : //  15cm -  20cm
                   value >= 389 ?  25 : //  20cm -  30cm
                   value >= 298 ?  35 : //  30cm -  40cm
                   value >= 240 ?  45 : //  40cm -  50cm
                   value >= 204 ?  55 : //  50cm -  60cm
                   value >= 173 ?  65 : //  60cm -  70cm
                   value >= 150 ?  75 : //  70cm -  80cm
                   value >= 131 ?  85 : //  80cm -  90cm
                   value >= 119 ?  95 : //  90cm - 100cm
                   value >= 102 ? 105 : // 100cm - 110cm
                   value >=  94 ? 115 : // 110cm - 120cm
                   value >=  82 ? 125 : // 120cm - 130cm
                   value >=  73 ? 135 : // 130cm - 140cm
                   value >=  67 ? 145 : // 140cm - 150cm
                   -1;

       // 物体までの距離が設定した検出範囲内の場合のみ検出したと見なす
       if (distance >= DETECTING_DIST_MIN && distance <= DETECTING_DIST_MAX && distance >= INFRAERD_DIST_MIN && distance <= INFRAERD_DIST_MAX) {
           isDetecting = true;
           intensity = 1 - (distance - DETECTING_DIST_MIN) / (DETECTING_DIST_MAX - DETECTING_DIST_MIN);
       }

       // 検出に失敗した場合
       if (!isDetecting) {
            distance = DETECTING_DIST_MAX;
            intensity = 0;
        }

        // シリアルモニタにデバッグ出力する
        if (!RELEASE_MODE) {
            Serial.print(index);
            Serial.print(" ");
            Serial.print(value);
            Serial.print(" ");
            Serial.println(distance);
        }
    }

private:

    // 受信用ピン番号
    int outputPin;
};





/* -------------------------------------------------- *
 *
 * アプリケーション全体をとりまとめるクラス
 *
 * -------------------------------------------------- */
class Application {
public:

    Application() {
        sensorCount = 0;
    }

    // 初期化
    void setup() {
        // ボードとのシリアル通信を開始する
        Serial.begin(9600);

        // センサを登録する（赤外線距離センサは超音波センサに対してノイズを与えるため不使用）
        //
        // 超音波距離センサについて
        //   HC-SR04を使用していますが、2015/7/23時点での最新ロットにバグが存在するので注意してください。
        //   http://mag.switch-science.com/2015/07/23/hc-sr04_failure/
        //
        addSensor(new UltrasonicSensor(2,  8)); // Trig端子をデジタル2番ピンに接続、Echo端子をデジタル8番ピンに接続
        // addSensor(new InfraredSensor(0));    // 出力端子をアナログ入力0番ピンに接続
        addSensor(new UltrasonicSensor(3,  9)); // Trig端子をデジタル3番ピンに接続、Echo端子をデジタル9番ピンに接続
        // addSensor(new InfraredSensor(1));    // 出力端子をアナログ入力1番ピンに接続
        addSensor(new UltrasonicSensor(4, 10)); // Trig端子をデジタル4番ピンに接続、Echo端子をデジタル10番ピンに接続
        // addSensor(new InfraredSensor(2));    // 出力端子をアナログ入力2番ピンに接続
        addSensor(new UltrasonicSensor(5, 11)); // Trig端子をデジタル5番ピンに接続、Echo端子をデジタル11番ピンに接続
        // addSensor(new InfraredSensor(3));    // 出力端子をアナログ入力3番ピンに接続
        addSensor(new UltrasonicSensor(6, 12)); // Trig端子をデジタル6番ピンに接続、Echo端子をデジタル12番ピンに接続
        // addSensor(new InfraredSensor(4));    // 出力端子をアナログ入力4番ピンに接続
        addSensor(new UltrasonicSensor(7, 13)); // Trig端子をデジタル7番ピンに接続、Echo端子をデジタル13番ピンに接続

        // センサのレイアウトを計算して初期化する（センサはセンサボックス内で等間隔に配置されているとする）
        float margin = SENSOR_W / (sensorCount - 1);
        for (int i = 0; i < sensorCount; ++i) {
            Sensor* sensor = sensors[i];
            float localPositionX = i * margin;
            float localPositionY = 0;
            float globalPositionX = SENSOR_X + localPositionX;
            float globalPositionY = SENSOR_Y + localPositionY;
            sensor->setup(i, localPositionX, localPositionY, globalPositionX, globalPositionY);
        }

        // 物体検出フラグの初期化
        isDetecting = false;
        isDetectingNoise = false;
        lastDetectingCount = detectingCount = 0;
        lastDetectedTime = 0;
        detectingNoiseStartTime = 0;

        // 物体検出座標の初期化
        resetPersonTargetPosition();
        personX = personTargetX;
        personY = personTargetY;
        personZ = personTargetZ;
        nextLookAtYSetTime = 0;
    }

    // 更新
    void loop() {
        int currentTime = millis();

        int nearestIndex = -1;
        float nearestDistance = DETECTING_DIST_MAX;
        float nearestIntensity = 0;

        float lookAtX;
        float lookAtY;

        isDetecting = false;
        lastDetectingCount = detectingCount;
        detectingCount = 0;

        for (int i = 0; i < sensorCount; ++i) {
            Sensor* sensor = sensors[i];

            // センサを更新する
            sensor->update();

            // センサが物体を検出しているかどうかチェックする
            if (sensor->isDetecting) {
                isDetecting = true;
                ++detectingCount;

                // 検出した物体がモニタに最も近い場合は記録する
                if (sensor->intensity > nearestIntensity) {
                    nearestIndex= i;
                    nearestIntensity = sensor->intensity;
                    nearestDistance = sensor->distance;
                }
            }
        }

        // 検出数0の状態から新規に検出した場合、検出してから一定時間はノイズとしてみなして未検出として扱う
        if (isDetectingNoise) {
            if (currentTime - detectingNoiseStartTime < DETECTING_NOISE_DELAY * 1000) {
                isDetecting = false;
            } else {
                isDetectingNoise = false;
            }
        } else if (lastDetectingCount == 0 && detectingCount > 0) {
            detectingNoiseStartTime = currentTime;
            isDetectingNoise = true;
        }

        // 物体を検出している場合、目標座標を更新する
        if (isDetecting) {
            lastDetectedTime = currentTime;
            Sensor *nearestSensor = sensors[nearestIndex];
            personTargetX = nearestSensor->globalPositionX;
            personTargetY = nearestSensor->globalPositionY + (float)nearestSensor->distance;
        }

        // 高さ方向に対してはランダムな座標を割り当てる
        if (currentTime >= nextLookAtYSetTime) {
            personTargetZ = 0.5 + random(-1, 1) * LOOK_AT_Y_RANGE;
            nextLookAtYSetTime = currentTime + random(LOOK_AT_Y_INTERVAL_MIN, LOOK_AT_Y_INTERVAL_MAX) * 1000;
        }

        // 検出座標を目標座標に対して動かす
        personX += (personTargetX - personX) * POSITION_EASING;
        personY += (personTargetY - personY) * POSITION_EASING;
        personZ += (personTargetZ - personZ) * POSITION_EASING;

        // 現在検出していない場合でも、未検出になってから指定時間以内であれば前回検出した時点での検出座標を採用しつつ検出していることにする
        if (!isDetecting && (currentTime - lastDetectedTime < DETECTING_LOST_DELAY * 1000)) {
            isDetecting = true;
        }

        if (isDetecting) {

            // 検出座標をモニタ座標に変換する
            if (LOOK_AT_X_BY_ANGLE) {
                float lookAtAngle = atan2(personY - MONITOR_CENTER_Y, personX - MONITOR_CENTER_X);
                lookAtX = 1 - lookAtAngle / PI;
            } else {
                lookAtX = (personX - SENSOR_X) / (SENSOR_W);
            }
            lookAtY = 1 - personZ;

        } else {

            // 未検出の場合は-1をセットする
            resetPersonTargetPosition();
            lookAtX = -1;
            lookAtY = -1;
        }

        // 結果をシリアル通信で送信する
        if (RELEASE_MODE) {

            // モニタ上でのxy座標を送信する
            Serial.print(lookAtX);
            Serial.print(",");
            Serial.print(lookAtY);

            // ----------
            // この部分はデバッグのために出力しているだけ
            Serial.print(",");
            Serial.print(sensorCount);
            Serial.print(",");
            Serial.print(nearestIndex);
            Serial.print(",");
            Serial.print(nearestDistance);
            Serial.print(",");
            Serial.print(nearestIntensity);
            Serial.print(",");
            Serial.print(personX);
            Serial.print(",");
            Serial.print(personY);
            Serial.print(",");
            Serial.print(personZ);
            for (int i = 0; i < sensorCount; ++i) {
                Sensor* sensor = sensors[i];
                Serial.print(",");
                Serial.print(sensor->globalPositionX);
                Serial.print(":");
                Serial.print(sensor->globalPositionY);
                Serial.print(":");
                Serial.print(sensor->distance);
                Serial.print(":");
                Serial.print(sensor->intensity);
            }
            // ----------

            Serial.print("\n");
        }
    }

private:

    // センサを追加登録する（setupの指定位置で呼び出すこと）
    void addSensor(Sensor *sensor) {
        sensors[sensorCount++] = sensor;
    }

    // 目標座標をリセットする
    void resetPersonTargetPosition() {
        personTargetX = MONITOR_CENTER_X;
        personTargetY = MONITOR_CENTER_Y;
        personTargetZ = 0.5;
    }

    // センサ配列
    Sensor *sensors[100];

    // センサ数
    int sensorCount;

    // 検出フラグ
    boolean isDetecting;
    boolean isDetectingNoise;
    int detectingCount;
    int lastDetectingCount;
    int lastDetectedTime;
    int detectingNoiseStartTime;

    // 検出座標
    float personTargetX;
    float personTargetY;
    float personTargetZ;
    float personX;
    float personY;
    float personZ;

    // 高さ方向にランダムな値をセットする目的
    int nextLookAtYSetTime;
};





// --------------------------------------------------
// エントリーポイント
Application application;

void setup() {
  application.setup();
}

void loop() {
  application.loop();
}

