# website-personal

Kumpulan proyek pribadi berbasis IoT dan AI:

1. **Website monitoring "KING RON IOT"** (Flask): dashboard suhu, kelembapan, dan kelembapan tanah.
2. **ESP32 Voice Assistant**: ESP32 merekam suara, mengirimnya ke server, lalu memutar balasan audio.
3. **YOLOv8 Detector**: deteksi objek lewat webcam beserta estimasi ukuran dan warna dominan.

## Struktur Proyek

```
website-personal/
├── WEBSITE GWEH/          # Website Flask
│   ├── app.py
│   ├── static/            # Gambar dan video intro
│   └── templates/
│       ├── index.html
│       ├── monitoring.html
│       └── datalog.html
├── src/
│   └── main.cpp           # Kode ESP32 voice assistant
├── platformio.ini         # Konfigurasi PlatformIO
├── yolo_detect.py         # Skrip deteksi objek YOLOv8
├── include/
├── lib/
└── test/
```

## 1. Website Monitoring (Flask)

Website dengan tiga halaman:

| Halaman | Route | Isi |
|---|---|---|
| Home | `/` | Intro, menu, profil sensor (ESP8266, DHT11, soil moisture, relay), dan timeline |
| Monitoring | `/monitoring` | Nilai live dan grafik suhu serta kelembapan 24 jam (Chart.js) |
| Datalog | `/datalog` | Tabel riwayat data sensor |

> **Status:** data pada halaman Monitoring dan Datalog saat ini masih **data dummy** (nilai acak), belum terhubung ke sensor sungguhan.

### Cara menjalankan

```bash
cd "WEBSITE GWEH"
pip install flask
python app.py
```

Buka `http://localhost:5000` di browser. Untuk mengakses dari HP, hubungkan HP ke WiFi yang sama lalu buka `http://<IP-komputer>:5000`.

## 2. ESP32 Voice Assistant

Alur kerja:

1. Tekan tombol, ESP32 berbunyi "beep" lalu merekam suara selama 5 detik (16 kHz, 16-bit, mono).
2. Rekaman dikemas sebagai file WAV dan dikirim lewat HTTP POST ke server (`/process-audio`).
3. Server membalas dengan audio MP3, lalu ESP32 memutarnya lewat speaker.

### Perangkat keras

| Komponen | Fungsi |
|---|---|
| ESP32 | Mikrokontroler |
| INMP441 | Mikrofon I2S |
| MAX98357A | Amplifier I2S untuk speaker |
| Push button | Pemicu rekaman |

### Wiring

| Fungsi | GPIO |
|---|---|
| Mic WS | 15 |
| Mic SCK | 14 |
| Mic SD | 32 |
| Speaker WS (LRC) | 25 |
| Speaker BCLK | 26 |
| Speaker DOUT (DIN) | 22 |
| Tombol (ke GND, `INPUT_PULLUP`) | 33 |

### Konfigurasi

Atur SSID, password WiFi, dan alamat server di `src/main.cpp`:

```cpp
const char* ssid = "NAMA_WIFI";
const char* password = "PASSWORD_WIFI";
const char* serverUrl = "http://<IP-SERVER>:5000/process-audio";
```

> Jangan commit kredensial WiFi asli ke repo publik. Simpan di file terpisah (mis. `secrets.h`) yang dimasukkan ke `.gitignore`.

### Build dan upload

1. Install VS Code dan ekstensi **PlatformIO IDE**.
2. Buka folder repo ini, lalu klik **Upload**.
3. Buka **Serial Monitor** (115200 baud) untuk melihat log.

## 3. YOLOv8 Object Detector

Mendeteksi objek dari webcam memakai YOLOv8n, lalu menampilkan:

- nama objek dan tingkat keyakinan (confidence)
- warna dominan objek
- estimasi lebar dan tinggi (cm) berdasarkan kalibrasi jarak kamera 30 cm
- FPS

### Cara menjalankan

```bash
pip install ultralytics opencv-python numpy
python yolo_detect.py
```

Tekan `q` untuk keluar. Ubah `DISTANCE_CM` di skrip sesuai jarak kamera ke objek agar estimasi ukuran lebih akurat.

## Catatan

- Estimasi ukuran pada YOLO bersifat perkiraan karena memakai skala piksel ke cm yang sederhana.
- Server penerima audio untuk ESP32 belum termasuk di repo ini.
