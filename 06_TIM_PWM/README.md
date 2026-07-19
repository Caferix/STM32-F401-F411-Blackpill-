# 06\_TIM\_PWM — Hardware PWM Motor Speed Control / Donanımsal PWM Motor Hız Kontrolü

> 🇬🇧 [English](#english) | 🇹🇷 [Türkçe](#türkçe)

---

## English

This project implements smooth, non-blocking DC motor speed control using STM32's TIM2 hardware timer in PWM mode. A potentiometer connected to ADC1 sets the target speed; a ramp algorithm gradually moves the motor toward it — without a single `HAL_Delay` in the loop.

### 💬 What I Learned

> *I learned that not every peripheral on the STM32F411 runs at the same clock speed — data buses (APB1 and APB2) have their own speed limits, and each peripheral is fed from the appropriate one.*

> *I learned how to synchronize the hardware registers inside the processor (PSC, CNT, ARR) to define the period and resolution of a PWM signal. We divided the 100 MHz clock by 5 using PSC = 4 (because of the hardware +1 rule). The counter (CNT) then counts from 0 up to our ceiling of ARR = 999. After completing 1000 steps from 0 to 999, an update event (UEV) fires and the counter resets. This cycle repeats 20,000 times per second, producing a 20 kHz ultrasonic PWM period that is inaudible to the human ear, with a speed resolution of 0.1%.*

> *I learned why electrical glitches can occur when changing speed while the motor is running, and the architectural way to prevent them. If a new speed value is written directly to the register the counter is actively reading, the counter can lose track of where to stop and generate signal errors that jolt the motor. STM32 prevents this using Preload (staging) and Shadow (active) register layers. By enabling the ARPE bit, we ensured that any new speed value is held in a waiting room first and only transferred to the active register when the counter cleanly resets at a UEV — guaranteeing glitch-free transitions.*

> *I learned how to safely map an external element (potentiometer) to motor speed without locking the processor (no HAL_Delay). The 12-bit raw analog value read from the potentiometer via ADC1 (0–4095) is bridged to the motor's PWM range (0–999) through a linear ratio formula. Thanks to the HAL_GetTick()-based ramp engine, the motor reaches its target speed in safe millisecond steps rather than jumping instantly — protecting both the system and unsigned integers from underflow. Most importantly, since all signal generation is handled independently by hardware units in the background, the CPU core remains completely free.*

### 🎯 Goals

- Replace software PWM (CPU-generated) with hardware timer PWM (TIM2).
- Understand ARR and CCR registers and their relationship to frequency and duty cycle.
- Select PWM frequency deliberately — 20 kHz for ultrasonic silent switching.
- Read analog input via ADC and map it linearly to motor speed range.
- Implement a non-blocking ramp with unsigned integer underflow protection.
- Enforce defensive clamping through a single hardware-access wrapper function.

### 🕹️ How It Works

**Hardware Layer — TIM2 PWM**

TIM2 Channel 1 drives PA0 (connected to SW-M221 MOSFET gate). The timer counts from 0 to ARR continuously; when CNT < CCR the pin is HIGH, when CNT ≥ CCR the pin is LOW. CPU never touches the GPIO directly — only CCR is updated.

| Parameter | Value | Result |
| :--- | :--- | :--- |
| TIM2CLK | 100 MHz | APB1 × 2 multiplier |
| PSC | 4 | CK_CNT = 100MHz / 5 = 20 MHz |
| ARR | 999 | f_PWM = 20MHz / 1000 = **20 kHz** |
| CCR range | 0 – 999 | 1000-step speed resolution |

20 kHz was chosen deliberately — above the human hearing threshold, so the motor runs silently.

**Software Layer — Main Loop**

Three things happen every loop iteration:

```
1. ADC read     → adc_raw (0–4095)
2. Linear map   → target_speed = (adc_raw × 999) / 4095
3. Ramp update  → every 10 ms, current_speed steps ±5 toward target
```

**Motor Controller Struct**

All motor state is held in one place:

```c
typedef struct {
    uint16_t current_speed;     // Active CCR value (0–999)
    uint16_t target_speed;      // Desired CCR value from ADC
    uint32_t last_tick;         // Timestamp of last ramp step
    uint32_t step_interval_ms;  // Ramp cadence (10 ms)
} MotorController_t;
```

**Defensive Clamping — Motor\_Set\_Speed**

The only function that writes to hardware. No matter what value arrives, the CCR register can never exceed ARR:

```c
void Motor_Set_Speed(uint16_t duty) {
    if (duty > 999) duty = 999;
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, duty);
}
```

**Ramp & Underflow Protection**

`current_speed` approaches `target_speed` in steps of 5. When the gap is less than 5, it snaps directly to the target — preventing infinite oscillation. On the decreasing side, the subtraction is guarded before execution to prevent `uint16_t` underflow (which would wrap to ~65000):

```c
if (motor.current_speed - motor.target_speed < 5)
    motor.current_speed = motor.target_speed;  // snap, don't subtract
else
    motor.current_speed -= 5;
```

### 🔧 Wiring

| Component | Pin | Note |
| :--- | :--- | :--- |
| SW-M221 PWM IN | PA0 (TIM2\_CH1) | AF push-pull, no pull |
| Potentiometer wiper | PA1 (ADC1\_IN1) | Analog mode |
| Potentiometer ends | 3.3V / GND | Blackpill onboard supply |
| Motor | Vout+/Vout− | SW-M221 output terminals |
| 5V supply | Vin+/Vin− | SW-M221 input terminals |
| Flyback diode | Across motor | External — protects MOSFET |

ST-Link wiring:

| ST-Link V2 | Blackpill |
| :--- | :--- |
| 3.3V | 3.3V |
| GND | GND |
| SWDIO | SWDIO |
| SWCLK | SWCLK |

### 💻 Tools Used

| Tool | Detail |
| :--- | :--- |
| **Language** | C |
| **Framework** | STM32 HAL |
| **Build System** | Makefile |
| **Flash Tool** | st-flash v1.7.0 |
| **System Clock** | 100 MHz (HSE 25 MHz, PLL: M=25 N=400 P=4) |
| **PWM Frequency** | 20 kHz (PSC=4, ARR=999) |

### 🔍 Key Concepts

- **ARR** — sets PWM period; determines frequency and speed resolution
- **CCR** — sets duty cycle; `Motor_Set_Speed()` is the only place it is written
- **f_PWM = TIM_CLK / ((PSC+1) × (ARR+1))** — 100MHz / 5 / 1000 = 20 kHz
- **20 kHz** — above human hearing; motor runs without audible whine
- **Linear mapping** — `(adc_raw × 999) / 4095` scales 12-bit ADC to CCR range
- **Ramp** — `current_speed` steps toward `target_speed` every 10 ms; prevents current spikes
- **Underflow guard** — gap check before subtraction prevents `uint16_t` wraparound
- **Defensive clamping** — single wrapper enforces hardware limits regardless of caller

---

## Türkçe

Bu proje, STM32'nin TIM2 donanım zamanlayıcısını PWM modunda kullanarak pürüzsüz, non-blocking DC motor hız kontrolü uygular. ADC1'e bağlı potansiyometre hedef hızı belirler; rampa algoritması motoru yavaş yavaş hedefe götürür — döngüde tek bir `HAL_Delay` yoktur.

### 💬 Öğrendiklerim

> *STM32F411 mikrodenetleyicisinde her çevre biriminin aynı hızda çalışamadığını, veri yollarının (APB1 ve APB2) hız sınırları olduğunu öğrendim.*

> *PWM sinyalinin periyodunu ve çözünürlüğünü belirlemek için işlemcinin içindeki donanımsal yazmaçları (PSC, CNT, ARR) nasıl senkronize edeceğimi öğrendim. 100 MHz'lik hızı PSC = 4 girerek 5'e böldük (donanımdaki +1 kuralı yüzünden). Sayaç (CNT) bu yeni hızda 0'dan başlayıp tepe sınırımız olan ARR = 999 değerine kadar sayar. 0-999 arası toplam 1000 adım atıldığında bir alarm (UEV) tetiklenir ve sayaç sıfırlanır. Bu döngü saniyede 20.000 kez tekrarlanarak motorun insan kulağı tarafından duyulmayan, sessiz 20 kHz ultrasonik PWM periyodunu ve %0.1 hassasiyetli hız çözünürlüğünü oluşturur.*

> *Motor dönerken hız değiştirdiğimizde donanımda neden elektriksel sıçramalar (glitch) oluşabileceğini ve bunu önlemenin mimari yolunu öğrendim. Çalışma zamanında yeni bir hız değeri yazdığımızda, bu değer doğrudan sayacın o an okuduğu devreye giderse sayaç duracağı yeri şaşırıp kilitlenebilir ve motoru sarsacak sinyal hataları üretebilir. STM32 bunun önüne geçmek için Preload (Bekleme) ve Shadow (Gölge/Çalışma) katmanlarını kullanır. Biz ARPE bitini aktif ederek yeni hız değerinin önce bekleme odasında tutulmasını, sayaç ancak temiz bir şekilde sıfırlanırken (UEV anında) gerçek çalışma odasına aktarılmasını sağladık; böylece pürüzsüz geçişi garantiledik.*

> *İşlemciyi kilitlemeden (HAL_Delay kullanmadan) harici bir elemanla (potansiyometre) motor hızını nasıl güvenle eşleyeceğimi öğrendim. Potansiyometreden ADC1 ile okuduğumuz 12-bitlik ham analog veriyi (0-4095 arası), matematiksel bir oran-orantı köprüsüyle motorun PWM sınırlarına (0-999 arası) doğrusal olarak haritaladık. HAL_GetTick() tabanlı yazılan rampa motoru sayesinde, bu hedef hıza aniden fırlamak yerine milisaniyelik güvenli adımlarla, sisteme ve işaretsiz sayılara (unsigned integer underflow) zarar vermeden pürüzsüzce ulaştık. En önemlisi, tüm sinyal üretimi arka planda bağımsız donanım birimlerince yapıldığı için işlemci çekirdeğimiz tamamen özgür kaldı.*

### 🎯 Hedefler

- Yazılımsal PWM'i (CPU tabanlı) donanım timer PWM'iyle (TIM2) değiştirmek.
- ARR ve CCR register'larını ve bunların frekans ile duty cycle ilişkisini anlamak.
- PWM frekansını bilinçli seçmek — ultrasonik sessiz anahtarlama için 20 kHz.
- ADC ile analog giriş okuyup motor hız aralığına doğrusal olarak eşlemek.
- İşaretsiz tam sayı underflow korumasıyla non-blocking rampa uygulamak.
- Tek bir donanım erişim sarmalayıcı fonksiyonla savunmacı sınırlama sağlamak.

### 🕹️ Nasıl Çalışır?

**Donanım Katmanı — TIM2 PWM**

TIM2 Kanal 1, PA0 pinini (SW-M221 MOSFET kapısına bağlı) sürer. Timer 0'dan ARR'ye sürekli sayar; CNT < CCR iken pin HIGH, CNT ≥ CCR iken pin LOW olur. CPU GPIO'ya hiç dokunmaz — yalnızca CCR güncellenir.

| Parametre | Değer | Sonuç |
| :--- | :--- | :--- |
| TIM2CLK | 100 MHz | APB1 × 2 çarpanı |
| PSC | 4 | CK_CNT = 100MHz / 5 = 20 MHz |
| ARR | 999 | f_PWM = 20MHz / 1000 = **20 kHz** |
| CCR aralığı | 0 – 999 | 1000 basamak hız çözünürlüğü |

20 kHz bilinçli seçildi — insan işitme eşiğinin üzerinde, motor sessiz çalışır.

**Yazılım Katmanı — Ana Döngü**

Her döngü iterasyonunda üç şey olur:

```
1. ADC oku      → adc_raw (0–4095)
2. Doğrusal eşle → target_speed = (adc_raw × 999) / 4095
3. Rampa güncelle → her 10 ms'de current_speed ±5 adım hedefe yaklaşır
```

**Motor Kontrolcü Struct'ı**

Tüm motor durumu tek bir yerde tutulur:

```c
typedef struct {
    uint16_t current_speed;     // Aktif CCR değeri (0–999)
    uint16_t target_speed;      // ADC'den gelen hedef CCR değeri
    uint32_t last_tick;         // Son rampa adımının zaman damgası
    uint32_t step_interval_ms;  // Rampa kadansı (10 ms)
} MotorController_t;
```

**Savunmacı Sınırlama — Motor\_Set\_Speed**

Donanıma yazan tek fonksiyon. Ne değer gelirse gelsin CCR register'ı ARR'yi asla aşamaz:

```c
void Motor_Set_Speed(uint16_t duty) {
    if (duty > 999) duty = 999;
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, duty);
}
```

**Rampa ve Underflow Koruması**

`current_speed`, `target_speed`'e 5'er adımla yaklaşır. Fark 5'ten azsa direkt hedefe atlar — sonsuz salınımı önler. Azalma tarafında çıkarma işlemi yapılmadan önce fark kontrol edilir; bu `uint16_t` underflow'u (yaklaşık 65000'e sarmalama) engeller:

```c
if (motor.current_speed - motor.target_speed < 5)
    motor.current_speed = motor.target_speed;  // atla, çıkarma
else
    motor.current_speed -= 5;
```

### 🔧 Bağlantı Şeması

| Bileşen | Pin | Not |
| :--- | :--- | :--- |
| SW-M221 PWM IN | PA0 (TIM2\_CH1) | AF push-pull, pull yok |
| Potansiyometre orta uç | PA1 (ADC1\_IN1) | Analog mod |
| Potansiyometre uçları | 3.3V / GND | Blackpill beslemesinden |
| Motor | Vout+/Vout− | SW-M221 çıkış klemenslerine |
| 5V kaynak | Vin+/Vin− | SW-M221 giriş klemenslerine |
| Flyback diyot | Motor uçları arasına | Harici — MOSFET koruma |

ST-Link bağlantısı:

| ST-Link V2 | Blackpill |
| :--- | :--- |
| 3.3V | 3.3V |
| GND | GND |
| SWDIO | SWDIO |
| SWCLK | SWCLK |

### 💻 Kullanılan Araçlar

| Araç | Detay |
| :--- | :--- |
| **Dil** | C |
| **Framework** | STM32 HAL |
| **Build System** | Makefile |
| **Flash Tool** | st-flash v1.7.0 |
| **Sistem Saati** | 100 MHz (HSE 25 MHz, PLL: M=25 N=400 P=4) |
| **PWM Frekansı** | 20 kHz (PSC=4, ARR=999) |

### 🔍 Temel Kavramlar

- **ARR** — PWM periyodunu belirler; frekansı ve hız çözünürlüğünü ayarlar
- **CCR** — duty cycle'ı belirler; yalnızca `Motor_Set_Speed()` içinde yazılır
- **f_PWM = TIM_CLK / ((PSC+1) × (ARR+1))** — 100MHz / 5 / 1000 = 20 kHz
- **20 kHz** — insan işitme üstü; motor gürültüsüz çalışır
- **Doğrusal eşleme** — `(adc_raw × 999) / 4095` ile 12-bit ADC değeri CCR aralığına çekilir
- **Rampa** — `current_speed` her 10 ms'de `target_speed`'e 5 adım yaklaşır; ani akım çekişini önler
- **Underflow koruması** — çıkarmadan önce fark kontrolü `uint16_t` sarmasını engeller
- **Savunmacı sınırlama** — tek sarmalayıcı fonksiyon donanım limitini her koşulda zorlar