# 05\_LED\_Dimmer — Software PWM / Yazılımsal PWM

> 🇬🇧 [English](#english) | 🇹🇷 [Türkçe](#türkçe)

---

## English

This project explores three different approaches to generating PWM signals in software, progressively uncovering the tradeoffs between timing precision, CPU utilization, and code flexibility — all without using a hardware timer peripheral.

### 💬 What I Learned

> * Software PWM and the "Blocking Code" Dilemma: I experienced firsthand how using "busy-wait" functions like delay_us() for microsecond-level timing can completely paralyze a system. This blocking approach was the exact reason why button presses were missed during the brightness adjustments in Stage B. It taught me the critical importance of writing non-blocking code in embedded software to ensure the processor never becomes "blind and deaf" to external events.

> * Hardware Tick Precision with DWT: Since the 1ms resolution of the standard HAL_GetTick() function falls short for high-frequency PWM generation, I explored the DWT (Data Watchpoint and Trace) unit embedded within the ARM Cortex-M core. By utilizing the DWT->CYCCNT register to track every single clock cycle, I successfully implemented a microsecond-accurate "Breathing LED" algorithm without freezing the processor or locking up the system.


> *  Harmonizing Different Time Scales: I learned how to orchestrate a microsecond-level PWM signal alongside a millisecond-level brightness update (dutyCycle modulation) within the same infinite loop. This project provided valuable experience in executing concurrent tasks across vastly different time horizons without allowing them to interfere with each other or compromise hardware availability.Harmonizing Different Time Scales: I learned how to orchestrate a microsecond-level PWM signal alongside a millisecond-level brightness update (dutyCycle modulation) within the same infinite loop. This project provided valuable experience in executing concurrent tasks across vastly different time horizons without allowing them to interfere with each other or compromise hardware availability.

### 🎯 Goals

- Understand duty cycle and PWM frequency from first principles.
- Implement and compare three software PWM approaches: `HAL_GetTick()` based, busy-wait loop based, and DWT cycle-counter based.
- Observe the CPU blocking problem of busy-wait PWM firsthand by attempting to read a button simultaneously.
- Discover the DWT (Data Watchpoint and Trace) cycle counter as a microsecond-precision timing tool.
- Implement a smooth breathing effect as a real-world duty cycle application.

### 🕹️ How It Works

Three approaches were implemented in sequence, each commented out as the next was added:

**Stage A — `HAL_GetTick()` Based PWM**

Uses millisecond tick modulo to determine the current phase within a 10 ms period. Non-blocking, but limited to ~100 Hz and coarse duty cycle steps due to 1 ms tick resolution.

```
phase = HAL_GetTick() % PWM_PERIOD
if phase < onTime → LED ON, else → LED OFF
```

Button reads duty cycle in 25% steps. Debounce handled with `HAL_Delay` (intentionally simple for this stage).

**Stage B — Busy-Wait Loop PWM**

Computes ON and OFF durations in microseconds, then calls `delay_us()` for each. Achieves finer timing but **completely blocks the CPU** — button reads placed after the loop are frequently missed.

```
delay_us(highTime)  → LED ON
delay_us(lowTime)   → LED OFF
// button check here is unreliable
```

This stage demonstrates the core limitation of software PWM: you cannot do anything else while the CPU is busy counting cycles.

**Stage C — DWT Cycle Counter PWM (Active)**

Uses `DWT->CYCCNT` directly to track position within a 10 ms period at CPU clock resolution (84 MHz = 84 cycles/µs). No blocking, no HAL dependency for timing.

```
totalTicks  = 10ms × 84 cycles/µs = 840 000 cycles
currentTick = DWT->CYCCNT % totalTicks
activeTicks = dutyCycle × totalTicks / 100
```

A separate `HAL_GetTick()` interval updates `dutyCycle` every 10 ms to produce a smooth 0 → 100 → 0 breathing animation.

### ⚠️ Notable Detail — Clock Source Changed

This project switches from HSI to **HSE (external crystal)** as the PLL source:

| Parameter | Value |
| :--- | :--- |
| Clock source | HSE (external, 25 MHz) |
| PLL M | 25 |
| PLL N | 168 |
| PLL P | 2 |
| **SYSCLK** | **84 MHz** |

`delay_us()` and `DWT`-based timing depend on `SystemCoreClock` being correct — CubeMX sets this automatically, but it is worth verifying when porting.

### 🔧 Wiring

| ST-Link V2 | Blackpill |
| :--- | :--- |
| 3.3V | 3.3V |
| GND | GND |
| SWDIO | SWDIO |
| SWCLK | SWCLK |

GPIO assignments are configured in CubeMX — see `gpio.c` for pin details. LEDs on `LED0–LED4` (GPIOA), button on `BUTTON_Pin` (GPIOB).

### 💻 Tools Used

| Tool | Detail |
| :--- | :--- |
| **Language** | C |
| **Framework** | STM32 HAL |
| **Build System** | Makefile |
| **Flash Tool** | st-flash v1.7.0 |
| **System Clock** | 84 MHz (HSE 25 MHz, PLL: M=25 N=168 P=2) |

### 🔍 Key Concepts

- **PWM** — rapid GPIO toggling creates an average voltage proportional to duty cycle: `V_avg = V × D`
- **`HAL_GetTick()` limit** — 1 ms resolution caps effective PWM frequency at ~100 Hz with coarse steps
- **Busy-wait cost** — `delay_us()` occupies 100% CPU; no other work can be done during the delay
- **DWT cycle counter** — `DWT->CYCCNT` counts every CPU clock cycle; enables µs precision without blocking
- **Breathing effect** — linearly ramping duty cycle 0 → 100 → 0 produces a smooth fade

---

## Türkçe

Bu proje, yazılımsal PWM sinyali üretmenin üç farklı yaklaşımını inceleyerek zamanlama hassasiyeti, CPU kullanımı ve kod esnekliği arasındaki dengeleri aşamalı olarak ortaya koyar — donanım timer çevre birimi kullanmadan.

### 💬 Öğrendiklerim

> * Yazılımsal PWM ve "Blocking Code" Çıkmazı: Mikrosaniye seviyesinde iş yaparken delay_us() gibi işlemciyi kilitleyen (blocking) fonksiyonların sistemi nasıl felç ettiğini gördüm. Aşama B'de parlaklığı ayarlarken buton basışlarının kaçma sebebi buydu. Gömülü yazılımda işlemciyi kör ve sağır etmeyen, "non-blocking" kod yazmanın kritik önemini yaşayarak öğrendim.

> * DWT ile Donanımsal Tik Hassasiyeti: HAL_GetTick() fonksiyonunun 1ms'lik çözünürlüğü yüksek frekanslı PWM için yetersiz kaldığından, ARM Cortex-M çekirdeğindeki DWT (Data Watchpoint and Trace) ünitesini keşfettim. DWT->CYCCNT register'ı sayesinde işlemcinin her saat çevrimini (clock cycle) sayarak, sistemi kilitlemeden mikrosaniye hassasiyetinde "Nefes Alan LED" algoritmasını kurmayı başardım.

> * Farklı Zaman Ölçeklerinin Uyumu: Mikro saniyelerle dönen PWM sinyali ile insan gözünün algılayacağı milisaniyelik parlaklık değişimini (dutyCycle güncellenmesi) aynı sonsuz döngü içinde, birbirini ezmeden ve donanımı askıya almadan eşzamanlı olarak yürütmeyi deneyimledim.

### 🎯 Hedefler

- Duty cycle ve PWM frekansını temel prensiplerden anlamak.
- Üç farklı yazılımsal PWM yaklaşımını uygulamak ve karşılaştırmak: `HAL_GetTick()` tabanlı, busy-wait döngü tabanlı ve DWT döngü sayacı tabanlı.
- Busy-wait PWM'in CPU engelleme sorununu aynı anda buton okumaya çalışarak bizzat gözlemlemek.
- DWT (Data Watchpoint and Trace) döngü sayacını mikrosaniye hassasiyetli bir zamanlama aracı olarak keşfetmek.
- Gerçek dünya duty cycle uygulaması olarak pürüzsüz bir nefes efekti gerçeklemek.

### 🕹️ Nasıl Çalışır?

Üç yaklaşım sırayla uygulandı; her biri bir sonraki eklenirken yorum satırına alındı:

**Aşama A — `HAL_GetTick()` Tabanlı PWM**

10 ms'lik periyot içindeki konumu belirlemek için milisaniye tick modülosu kullanır. Non-blocking olmakla birlikte, 1 ms tick çözünürlüğü nedeniyle ~100 Hz ile sınırlıdır ve duty cycle adımları kaba kalır.

```
phase = HAL_GetTick() % PWM_PERIOD
phase < onTime ise → LED AÇIK, değilse → LED KAPALI
```

Buton duty cycle'ı %25'lik adımlarla değiştirir. Debounce bu aşamada kasıtlı olarak basit tutuldu (`HAL_Delay`).

**Aşama B — Busy-Wait Döngü Tabanlı PWM**

ON ve OFF sürelerini mikrosaniye cinsinden hesaplar, ardından her biri için `delay_us()` çağırır. Daha ince zamanlama sağlar ama **CPU'yu tamamen bloke eder** — döngüden sonra yerleştirilen buton okumaları sık sık kaçırılır.

```
delay_us(highTime)  → LED AÇIK
delay_us(lowTime)   → LED KAPALI
// buton kontrolü burada güvenilmez
```

Bu aşama, yazılımsal PWM'in temel kısıtını gösterir: CPU meşgulken başka hiçbir şey yapılamaz.

**Aşama C — DWT Döngü Sayacı Tabanlı PWM (Aktif)**

10 ms'lik periyot içindeki konumu CPU saat çözünürlüğünde (84 MHz = 84 cycle/µs) takip etmek için doğrudan `DWT->CYCCNT` kullanır. Bloklama yok, zamanlama için HAL bağımlılığı yok.

```
totalTicks  = 10ms × 84 cycle/µs = 840 000 cycle
currentTick = DWT->CYCCNT % totalTicks
activeTicks = dutyCycle × totalTicks / 100
```

Ayrı bir `HAL_GetTick()` aralığı, pürüzsüz 0 → 100 → 0 nefes animasyonu üretmek için `dutyCycle`'ı her 10 ms'de bir günceller.

### ⚠️ Dikkat Edilmesi Gereken Detay — Saat Kaynağı Değişti

Bu proje HSI'den **HSE (harici kristal)** PLL kaynağına geçiş yapar:

| Parametre | Değer |
| :--- | :--- |
| Saat kaynağı | HSE (harici, 25 MHz) |
| PLL M | 25 |
| PLL N | 168 |
| PLL P | 2 |
| **SYSCLK** | **84 MHz** |

`delay_us()` ve DWT tabanlı zamanlama, `SystemCoreClock`'un doğru olmasına bağlıdır — bunu CubeMX otomatik ayarlar, ancak taşırken doğrulamak gerekir.

### 🔧 Bağlantı Şeması

| ST-Link V2 | Blackpill |
| :--- | :--- |
| 3.3V | 3.3V |
| GND | GND |
| SWDIO | SWDIO |
| SWCLK | SWCLK |

GPIO atamaları CubeMX'te yapılandırılmıştır — detaylar için `gpio.c` dosyasına bakın. LED'ler `LED0–LED4` (GPIOA), buton `BUTTON_Pin` (GPIOB) üzerinde.

### 💻 Kullanılan Araçlar

| Araç | Detay |
| :--- | :--- |
| **Dil** | C |
| **Framework** | STM32 HAL |
| **Build System** | Makefile |
| **Flash Tool** | st-flash v1.7.0 |
| **Sistem Saati** | 84 MHz (HSE 25 MHz, PLL: M=25 N=168 P=2) |

### 🔍 Temel Kavramlar

- **PWM** — hızlı GPIO geçişleri duty cycle ile orantılı ortalama gerilim üretir: `V_avg = V × D`
- **`HAL_GetTick()` sınırı** — 1 ms çözünürlük, PWM frekansını ~100 Hz ile kaba adımlarla sınırlar
- **Busy-wait maliyeti** — `delay_us()` CPU'nun %100'ünü kullanır; gecikme süresince başka hiçbir iş yapılamaz
- **DWT döngü sayacı** — `DWT->CYCCNT` her CPU saat döngüsünü sayar; bloklama olmadan µs hassasiyeti sağlar
- **Nefes efekti** — duty cycle'ı 0 → 100 → 0 doğrusal olarak rampalamak pürüzsüz bir solma üretir