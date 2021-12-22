#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <WiFiManager.h>
#include <Adafruit_NeoPixel.h>
#include <OneWire.h>
#include <DS18B20.h>
#include <Bounce2.h>

#include <math.h>
#include <array>

#ifndef STASSID
#define STASSID "lavalamp"
#define STAPSK "bugsbunny"
#endif

#define LED_MATRIX_WIDTH  8
#define LED_MATRIX_HEIGHT 11

#define WS2812B_DATA_PIN  14
#define WS2812B_COUNT     LED_MATRIX_WIDTH * LED_MATRIX_HEIGHT

#define DS18B20_DATA_PIN  4

#define BUGGY_D7
#ifdef BUGGY_D7
// Use D8 instead
#define BUTTON_PIN        15
#else
#define BUTTON_PIN        13
#endif

WiFiManager wifiManager;

OneWire oneWire(DS18B20_DATA_PIN);
DS18B20 sensor(&oneWire);

Adafruit_NeoPixel pixels(WS2812B_COUNT, WS2812B_DATA_PIN, NEO_GRB + NEO_KHZ800);

Bounce bounce;

double tempScale = 1.0;

struct Color
{
  uint8_t r, g, b;

  Color operator+(Color other)
  {
    return Color { r: r + other.r, g: g + other.g, b: b + other.b };
  }

  Color operator-(Color other)
  {
    return Color { r: r - other.r, g: g - other.g, b: b - other.b };
  }

  Color operator*(double a)
  {
    return Color { r: uint8_t(double(r) * a), g: uint8_t(double(g) * a), b: uint8_t(double(b) * a) };
  }

  friend Color operator*(double a, Color other)
  {
    return other * a;
  }
};

struct Vec3
{
  double x, y, z;

  Vec3 operator+(Vec3 other)
  {
    return Vec3 { x: x + other.x, y: y + other.y, z: z + other.z };
  }

  Vec3 operator-(Vec3 other)
  {
    return Vec3 { x: x - other.x, y: y - other.y, z: z - other.z };
  }

  double len()
  {
    return sqrt(x * x + y * y + z * z);
  }
};

Vec3 coords[WS2812B_COUNT] = { 0, };
double globalTime = 0.0;

void computeLedCoordinates()
{
  for (int i = 0; i < WS2812B_COUNT; i++)
  {
    int matrixX = i / LED_MATRIX_HEIGHT;
    int matrixY = matrixX % 2 == 0 ? (LED_MATRIX_HEIGHT - 1) - (i % LED_MATRIX_HEIGHT) : i % LED_MATRIX_HEIGHT;

    coords[i].x = 1.75 * sin(double(matrixX) * 2 * 3.14159 / double(LED_MATRIX_WIDTH));
    coords[i].y = 1.75 * cos(double(matrixX) * 2 * 3.14159 / double(LED_MATRIX_WIDTH));
    coords[i].z = 1.75 * (double(matrixY) - double(LED_MATRIX_HEIGHT) / 2.0);
  }
}

void setup()
{
  computeLedCoordinates();

  Serial.begin(115200);
  Serial.println("Booting...");

  WiFi.mode(WIFI_STA);

  wifiManager.autoConnect("Lavalamp", "12345678");

  ArduinoOTA.setHostname("lavalamp");
  ArduinoOTA.begin();

  sensor.begin();
  sensor.requestTemperatures();

  pixels.begin();

  bounce.attach(BUTTON_PIN,  INPUT_PULLUP);
  bounce.interval(5);
}

double clamp(double x, double min, double max)
{
  return x < min ? min : (x > max ? max : x);
}

Color lavaTemp(Vec3 p, double t)
{
  p.x += 2.5 * sin(0.07 * t);
  p.y += 2.5 * cos(0.11 * t);
  p.z += 2.0 * 5.5 * sin(0.2 * t);
  double d = sqrt(p.x * p.x + p.y * p.y + p.z * p.z);

  Color c = Color { r: uint8_t(tempScale * 15), g: 0, b: uint8_t((1.0 - tempScale) * 15) };
  double b1Size = 3.0;
  if (d < b1Size)
  {
    double b = (b1Size * b1Size - d * d) / (b1Size * b1Size);
    double colB = 256.0 * clamp(b, 0.0, 0.9);
    c = c + Color { r: uint8_t(tempScale * colB), g: uint8_t(180.0 * b), b: uint8_t((1.0 - tempScale) * colB) };
  }

  return c;
}

Color warmWhite(Vec3 p, double t)
{
  p.x += 2.2 * sin(0.07 * t);
  p.y += 2.2 * cos(0.11 * t);
  p.z += 2.0 * 5.5 * sin(0.2 * t);
  double d = sqrt(p.x * p.x + p.y * p.y + p.z * p.z);

  Color c = Color { r: 30, g: 18, b: 6 };
  double b1Size = 3.0;
  if (d < b1Size)
  {
    double b = (b1Size * b1Size - d * d) / (b1Size * b1Size);
    double colB = 256.0 * clamp(b, 0.0, 0.875);
    c = c + Color { r: colB, g: 0.8 * colB, b: 0.4 * colB };
  }

  return c;
}

Color ghana(Vec3 p, double t)
{
  double phi = atan2(p.y, p.x);
  double freq1 = 3.0;
  double freq2 = 0.2;
  double freq3 = 1.0;
  double freq4 = 5.0;
  double u = 0.5 + sin(freq1 * phi) / 2.0;
  double v = 0.5 + sin(freq2 * p.z + freq3 * t + freq4 * u) / 2.0;

  Color red { r: 100, g: 0, b: 0 };
  Color yellow { r: 50, g: 50, b: 0 };

  Color mix1 = u * red + (1.0 - u) * yellow;
  Color mix2 = v * mix1 + (1.0 - v) * 0.3 * yellow;

  return mix2;
}

Color snow(Vec3 p, double t)
{
  Vec3 flake1 = { x: -1.75, y: 0, z: fmod(1.0 * t, 40.0) - 9.0 };
  Vec3 flake2 = { x: 1.75, y: 0, z: fmod(1.0 * t, 40.0) - 13.0 };
  Vec3 flake3 = { x: 0, y: 1.75, z: fmod(1.0 * t, 40.0) - 19.0 };
  Vec3 flake4 = { x: -1.75, y: 0, z: fmod(1.0 * t, 40.0) - 23.0 };
  Vec3 flake5 = { x: 0, y: 1.75, z: fmod(1.0 * t, 40.0) - 27.0 };

  if (p.z > 5.0 && abs(p.x) < 0.5)
  {
    return Color { r: 255, g: 255, b: 255 };
  }
  else if (p.z > 7.0)
  {
    return Color { r: 180, g: 180, b: 180 };
  }

  double b = 0.0;

  Vec3 diffFlake1 = p - flake1;
  diffFlake1.x *= 2.0;
  diffFlake1.y *= 2.0;
  double distFlake1 = diffFlake1.len();

  if (distFlake1 < 2.0)
  {
    b += (4.0 - distFlake1 * distFlake1) / 4.0;
  }

  Vec3 diffFlake2 = p - flake2;
  diffFlake2.x *= 2.0;
  diffFlake2.y *= 2.0;
  double distFlake2 = diffFlake2.len();

  if (distFlake2 < 2.0)
  {
    b += (4.0 - distFlake2 * distFlake2) / 4.0;
  }

  Vec3 diffFlake3 = p - flake3;
  diffFlake3.x *= 2.0;
  diffFlake3.y *= 2.0;
  double distFlake3 = diffFlake3.len();

  if (distFlake3 < 2.0)
  {
    b += (4.0 - distFlake3 * distFlake3) / 4.0;
  }

  Vec3 diffFlake4 = p - flake4;
  diffFlake4.x *= 2.0;
  diffFlake4.y *= 2.0;
  double distFlake4 = diffFlake4.len();

  if (distFlake4 < 2.0)
  {
    b += (4.0 - distFlake4 * distFlake4) / 4.0;
  }

  Vec3 diffFlake5 = p - flake5;
  diffFlake5.x *= 2.0;
  diffFlake5.y *= 2.0;
  double distFlake5 = diffFlake5.len();

  if (distFlake5 < 2.0)
  {
    b += (4.0 - distFlake5 * distFlake5) / 4.0;
  }

  uint8_t colB = uint8_t(256.0 * clamp(b, 0.0, 1.0));
  return Color { r: colB, g: colB, b: colB };
}

Color dots(Vec3 p, double t)
{
  static Vec3 v;
  static double lastT = 0.0;

  if (t - lastT > 1.)
  {
    v = Vec3 {
      x: random(-9999, 9999) / 5000.0,
      y: random(-9999, 9999) / 5000.0,
      z: random(-9999, 9999) / 2000.0
    };
    lastT += 1.;
  }

  Color c = Color { r: 200, g: 0, b: 0 };

  if ((p - v).len() < 2.0)
  {
    return c;
  }

  return Color { r: 0, g: 0, b: 0 };
}

Color police(Vec3 p, double t)
{
  Vec3 v = Vec3 { x: 1.75 * cos(10.0 * t), y: 1.75 * sin(10.0 * t), z: -8.0 };
  Vec3 d = p - v;

  if (d.len() < 2.0)
  {
    double b = (2.0 - d.len()) / 2.0;
    uint8_t colB = uint8_t(256.0 * clamp(b * b, 0.0, 0.9));
    return Color { r: 0, g: 0, b: colB };
  }

  return { r: 0, g: 0, b: 0 };
}

void render(Color (*render_func)(Vec3, double))
{
  //uint8_t *data = pixels.getPixels();

  for (int i = 0; i < WS2812B_COUNT; i++)
  {
      Color c = (*render_func)(coords[i], globalTime);
      pixels.setPixelColor(i, pixels.Color(c.r, c.g, c.b));
      //pixels.setPixelColor(i, pixels.Color(0, 150, 0));
  }
}

void loop()
{
  static int mode = 0;

  bounce.update();
  if (bounce.changed())
  {
    if (bounce.read() == HIGH)
    {
      mode = (mode + 1) % 4;
    }
  }
  
  if (sensor.isConversionComplete())
  {
    //Serial.print("Temp: ");
    //Serial.println(sensor.getTempC());
    tempScale = (clamp(sensor.getTempC(), 16.0, 28.0) - 16.0) / 12.0;

    sensor.requestTemperatures();
  }

  pixels.clear();

  if (mode == 0)
  {
    render(warmWhite);
  }
  else if (mode == 1)
  {
    render(lavaTemp);
  }
  else if (mode == 2)
  {
    render(snow);
  }
  else if (mode == 3)
  {
    render(ghana);
  }
  else if (mode == 4)
  {
    render(police);
  }

  pixels.show();

  globalTime += 0.01;

  ArduinoOTA.handle();
}