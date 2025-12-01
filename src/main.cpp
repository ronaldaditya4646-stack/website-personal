#include <WiFi.h>
#include <HTTPClient.h>
#include <Audio.h>     // <--- pakai tanda < >
#include "driver/i2s.h"


// --- WAJIB GANTI ---
const char* ssid = "Fafa Kost 4g";
const char* password = "ebes_2104";

// --- INI SUDAH DI-UPDATE SESUAI IP SERVER KAMU ---
const char* serverUrl = "http://192.168.1.16:5000/process-audio";
// --------------------------------------------------


// --- Konfigurasi Pin ---
// Sesuaikan pin ini dengan board kamu jika beda

// --- PIN I2S MIC (INPUT - INMP441) ---
#define I2S_MIC_WS 15
#define I2S_MIC_SCK 14
#define I2S_MIC_SD 32

// --- PIN I2S SPEAKER (OUTPUT - MAX98357A) ---
#define I2S_SPEAKER_WS 25
#define I2S_SPEAKER_BCLK 26
#define I2S_SPEAKER_DOUT 22

// --- PIN TOMBOL ---
#define BUTTON_PIN 33 // Tombol terhubung ke GPIO 33 dan GND

// Pengaturan I2S Mic
#define SAMPLE_RATE 16000 // 16kHz
#define RECORD_DURATION_SEC 5 // Durasi rekam 5 detik
#define HEADER_SIZE 44
const int RECORD_BUFFER_SIZE = (SAMPLE_RATE * RECORD_DURATION_SEC * 2); // 16-bit audio (2 byte)
const int FULL_WAV_SIZE = RECORD_BUFFER_SIZE + HEADER_SIZE;

// Buffer untuk menyimpan rekaman audio (Makan RAM besar!)
byte* wavBuffer = NULL;

// Inisialisasi Audio Output (Speaker)
Audio audio;

// Fungsi untuk setup I2S Mic
void setupI2SMic() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT, // INMP441 itu mono
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_MIC_SCK,
    .ws_io_num = I2S_MIC_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_MIC_SD
  };

  i2s_driver_install((i2s_port_t)I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin((i2s_port_t)I2S_NUM_0, &pin_config);
  i2s_set_clk((i2s_port_t)I2S_NUM_0, SAMPLE_RATE, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);
}

// Fungsi untuk membuat header file WAV
// Ini penting agar server Python bisa baca file-nya
void createWavHeader(byte* header, int wavDataSize) {
  header[0] = 'R'; header[1] = 'I'; header[2] = 'F'; header[3] = 'F';
  unsigned int fileSize = wavDataSize + HEADER_SIZE - 8;
  header[4] = (byte)(fileSize & 0xFF);
  header[5] = (byte)((fileSize >> 8) & 0xFF);
  header[6] = (byte)((fileSize >> 16) & 0xFF);
  header[7] = (byte)((fileSize >> 24) & 0xFF);
  header[8] = 'W'; header[9] = 'A'; header[10] = 'V'; header[11] = 'E';
  header[12] = 'f'; header[13] = 'm'; header[14] = 't'; header[15] = ' ';
  header[16] = 16; header[17] = 0; header[18] = 0; header[19] = 0;
  header[20] = 1; header[21] = 0;
  header[22] = 1; header[23] = 0; // Mono
  header[24] = (byte)(SAMPLE_RATE & 0xFF);
  header[25] = (byte)((SAMPLE_RATE >> 8) & 0xFF);
  header[26] = 0; header[27] = 0;
  unsigned int byteRate = SAMPLE_RATE * 1 * (16 / 8); // 1 channel, 16 bits
  header[28] = (byte)(byteRate & 0xFF);
  header[29] = (byte)((byteRate >> 8) & 0xFF);
  header[30] = 0; header[31] = 0;
  header[32] = 2; header[33] = 0; // block align (channels * bits/sample / 8)
  header[34] = 16; header[35] = 0; // bits per sample
  header[36] = 'd'; header[37] = 'a'; header[38] = 't'; header[39] = 'a';
  header[40] = (byte)(wavDataSize & 0xFF);
  header[41] = (byte)((wavDataSize >> 8) & 0xFF);
  header[42] = (byte)((wavDataSize >> 16) & 0xFF);
  header[43] = (byte)((wavDataSize >> 24) & 0xFF);
}

// Fungsi utama: Rekam suara dan kirim ke server
void recordAndSend() {
  Serial.println("Mulai merekam...");
  
  // Alokasikan memori untuk buffer
  wavBuffer = (byte*)malloc(FULL_WAV_SIZE);
  if (wavBuffer == NULL) {
    Serial.println("Gagal alokasi memori buffer!");
    return;
  }
  
  byte* audioData = wavBuffer + HEADER_SIZE; // Arahkan pointer ke setelah header
  
  // Mulai merekam
  size_t bytes_read = 0;
  i2s_read((i2s_port_t)I2S_NUM_0, audioData, RECORD_BUFFER_SIZE, &bytes_read, portMAX_DELAY);

  if (bytes_read != RECORD_BUFFER_SIZE) {
    Serial.printf("Error rekaman. Harusnya %d bytes, tapi cuma %d bytes\n", RECORD_BUFFER_SIZE, bytes_read);
  } else {
    Serial.printf("Selesai merekam %d bytes.\n", bytes_read);
  }

  // Buat header WAV
  createWavHeader(wavBuffer, RECORD_BUFFER_SIZE);

  Serial.println("Mengirim audio ke server Python...");
  
  HTTPClient http;
  http.begin(serverUrl);
  http.addHeader("Content-Type", "audio/wav");
  http.setConnectTimeout(10000); // Timeout 10 detik

  // Kirim data WAV (header + audio)
  int httpResponseCode = http.POST(wavBuffer, FULL_WAV_SIZE);

  if (httpResponseCode == HTTP_CODE_OK) {
    Serial.println("Berhasil mengirim, server merespon OK.");
    
    // Dapatkan stream balasan (isinya MP3)
    WiFiClient* stream = http.getStreamPtr();
    
    Serial.println("Memainkan audio balasan...");
    
    // Alirkan stream MP3 balasan ke decoder
       if (audio.connecttoStream(stream, "audio/mpeg")) {
      Serial.println("Decoder MP3 terhubung. Memainkan...");
      // Loop ini akan 'blocking' (berhenti di sini) sampai MP3 selesai
      while (audio.isRunning()) {
        audio.loop(); // Handle audio playback
      }
      Serial.println("Selesai memainkan balasan.");
    } else {
      Serial.println("Gagal mengkoneksikan stream MP3 ke decoder.");
    }
    audio.stopSong(); // Pastikan berhenti

  } else {
    Serial.printf("Gagal mengirim. Error code: %d\n", httpResponseCode);
    String payload = http.getString();
    Serial.println("Respon error dari server:");
    Serial.println(payload);
  }

  http.end();
  free(wavBuffer); // Bebaskan memori RAM
  wavBuffer = NULL;
}


// --- SETUP ---
void setup() {
  Serial.begin(115200);
  Serial.println("Booting ESP32...");

  // Setup tombol
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Konek WiFi
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Setup I2S Mic
  setupI2SMic();

  // Setup I2S Speaker
  audio.setPinout(I2S_SPEAKER_BCLK, I2S_SPEAKER_WS, I2S_SPEAKER_DOUT);
  audio.setVolume(15); // Set volume (0-21), mulai dari kecil dulu
  
  Serial.println("\nSetup selesai. Tekan tombol untuk merekam...");
}


// --- LOOP ---
void loop() {
  // Cek apakah tombol ditekan (logika PULLUP: LOW berarti ditekan)
  if (digitalRead(BUTTON_PIN) == LOW) {
    Serial.println("Tombol ditekan!");
    
    // Kasih feedback suara "beep" sebelum rekam
    audio.tone(1000, 200); // Nada 1000Hz selama 200ms
    delay(200);
    
    // Mulai proses
    recordAndSend();
    
    Serial.println("\nMenunggu tombol ditekan lagi...");
    // Debounce: Tunggu sampai tombol dilepas + jeda
    while(digitalRead(BUTTON_PIN) == LOW) {
      delay(50);
    }
    delay(500); // Jeda agar tidak rekam terus-menerus
  }

  // Loop untuk library Audio (wajib ada meski kita blocking)
  // Ini menangani hal-hal seperti 'audio.tone()'
  if(audio.isRunning()) {
     audio.loop();
  }
}