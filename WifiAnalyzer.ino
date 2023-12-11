#include <WiFi.h>
#include <TFT_eSPI.h>

#define TFT_DC     2   // D1
#define TFT_CS     15  // D8
#define TFT_RST    4  // You may connect this to the reset pin on your ESP32 board, or -1 if not used

TFT_eSPI tft = TFT_eSPI(); // Invoke library

#define WIDTH 320
#define HEIGHT 240
#define BANNER_HEIGHT 16
#define GRAPH_BASELINE (HEIGHT - 18)
#define GRAPH_HEIGHT (HEIGHT - 52)
#define CHANNEL_WIDTH (WIDTH / 16)

#define RSSI_CEILING -40
#define RSSI_FLOOR -100
#define NEAR_CHANNEL_RSSI_ALLOW -70

uint16_t channel_color[] = {
  TFT_RED, TFT_ORANGE, TFT_YELLOW, TFT_GREEN, TFT_CYAN, TFT_MAGENTA,
  TFT_RED, TFT_ORANGE, TFT_YELLOW, TFT_GREEN, TFT_CYAN, TFT_MAGENTA,
  TFT_RED, TFT_ORANGE
};

uint8_t scan_count = 0;

void setup() {
  // Initialize TFT display
  tft.init();
  tft.setRotation(0);

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(0, 0);
  tft.print(" ESP32 WiFi Analyzer");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  delay(100);
}

void loop() {
  uint8_t ap_count[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  int32_t max_rssi[] = {-100, -100, -100, -100, -100, -100, -100, -100, -100, -100, -100, -100, -100, -100};

  int n = WiFi.scanNetworks();

  tft.fillRect(0, BANNER_HEIGHT, WIDTH, HEIGHT - BANNER_HEIGHT, TFT_BLACK);
  tft.setTextSize(1);

  if (n == 0) {
    tft.setTextColor(TFT_BLACK);
    tft.setCursor(0, BANNER_HEIGHT);
    tft.println("no networks found");
  } else {
    for (int i = 0; i < n; i++) {
      int32_t channel = WiFi.channel(i);
      int32_t rssi = WiFi.RSSI(i);
      uint16_t color = channel_color[channel - 1];
      int height = constrain(map(rssi, RSSI_FLOOR, RSSI_CEILING, 1, GRAPH_HEIGHT), 1, GRAPH_HEIGHT);

      ap_count[channel - 1]++;
      if (rssi > max_rssi[channel - 1]) {
        max_rssi[channel - 1] = rssi;
      }

      tft.drawLine(channel * CHANNEL_WIDTH, GRAPH_BASELINE - height, (channel - 1) * CHANNEL_WIDTH, GRAPH_BASELINE + 1, color);
      tft.drawLine(channel * CHANNEL_WIDTH, GRAPH_BASELINE - height, (channel + 1) * CHANNEL_WIDTH, GRAPH_BASELINE + 1, color);

      tft.setTextColor(color);
      tft.setCursor((channel - 1) * CHANNEL_WIDTH, GRAPH_BASELINE - 10 - height);
      tft.print(WiFi.SSID(i));
      tft.print('(');
      tft.print(rssi);
      tft.print(')');
      if (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) {
        tft.print('*');
      }

      delay(10);
    }
  }

tft.setTextColor(TFT_WHITE);
tft.setCursor(0, BANNER_HEIGHT);
tft.print(n);
tft.println(" networks found");

tft.print("Suggested channels: ");
bool listed_first_channel = false;
for (int i = 1; i <= 11; i++) {
    if ((i == 1) || (max_rssi[i - 2] < NEAR_CHANNEL_RSSI_ALLOW)) {
        if ((i == sizeof(channel_color) / sizeof(channel_color[0])) || (max_rssi[i] < NEAR_CHANNEL_RSSI_ALLOW)) {
            if (ap_count[i - 1] == 0) {
                if (!listed_first_channel) {
                    listed_first_channel = true;
                } else {
                    tft.print(", ");
                }
                tft.print(i);
            }
        }
    }
}

  tft.drawFastHLine(0, GRAPH_BASELINE, WIDTH, TFT_WHITE);
  for (int i = 1; i <= 14; i++) {
    tft.setTextColor(channel_color[i - 1]);
    tft.setCursor((i * CHANNEL_WIDTH) - ((i < 10) ? 3 : 6), GRAPH_BASELINE + 2);
    tft.print(i);
    if (ap_count[i - 1] > 0) {
      tft.setCursor((i * CHANNEL_WIDTH) - ((ap_count[i - 1] < 10) ? 9 : 12), GRAPH_BASELINE + 11);
      tft.print('(');
      tft.print(ap_count[i - 1]);
      tft.print(')');
    }
  }

  delay(1000);

  if (++scan_count >= 100) {
    ESP.deepSleep(0);
  }
}
