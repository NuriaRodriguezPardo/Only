#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "Audio.h"
#include <SD.h>
#include <Audio.h>
#include <SPI.h>
#include <Base64.h>

bool obtenerAudioDesdeSD(String &audioBase64);
String transcribeSpeech(const String &audioBase, const char* apiKey);

const char* ssid = "RedmiNuria";
const char* password = "Patata123";
const char* apiKey = "AIzaSyCVmhPyvDX0R8hTu6W8b4gBVqsRVIMYZOI";

// Función para conectar a WiFi
void connectToWiFi() {
    // WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, dns1, dns2);
    WiFi.begin(ssid, password);
    Serial.print("Conectando a WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("Conectado a la red WiFi");
}

void setup() {
    // Inicializar la comunicación serie
    Serial.begin(115200);
    // Comienza la conexión WiFi
    Serial.println();
    Serial.println();
    Serial.print("Conectando a ");
    Serial.println(ssid);

    // Inicializa la conexión WiFi
    WiFi.begin(ssid, password);

    // Espera hasta que la conexión se realice
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    // Una vez conectado, imprime la dirección IP asignada
    Serial.println("");
    Serial.println("WiFi conectado.");
    Serial.print("Dirección IP: ");
    Serial.println(WiFi.localIP());

    String audioBase;

    if (obtenerAudioDesdeSD(audioBase)) {
      Serial.println("Archivo de audio leído correctamente");
      // String audioBase64 = readAudioFileAndConvertToBase64(filename);
      // Convertir el audio a texto
      String transcribedText = transcribeSpeech(audioBase, apiKey);
      // Imprimir el texto transcrito
      Serial.println("Texto transcrito:");
      Serial.println(transcribedText);
      // Aquí puedes procesar el buffer de audio
    } else {
        Serial.println("Error al leer el archivo de audio");
    }
}
            

void loop() {
  // put your main code here, to run repeatedly:
}

const int chipSelect = 39; // Cambia esto según el pin que uses

bool obtenerAudioDesdeSD(String &audioBase64) {
  // Inicializar la tarjeta SD
  SPI.begin(36, 37, 35); // void begin(int8_t sck=-1, int8_t miso=-1, int8_t mosi=-1, int8_t ss=-1);
  if (!SD.begin(chipSelect)) {
    Serial.println("Error al montar la tarjeta SD");
    return false;
  }

  // Abrir el archivo de texto con el contenido Base64
  File archivo = SD.open("/pruebaBase64.txt", FILE_READ);
  if (!archivo) {
    Serial.println("Error al abrir el archivo de texto");
    return false;
  }

  // Leer el contenido del archivo de texto y almacenar en un String
  audioBase64 = "";
  while (archivo.available()) {
    char c = archivo.read();
    // Solo agregar caracteres válidos de Base64
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '+' || c == '/' || c == '=') {
      audioBase64 += c;
    }
  }

  // Cerrar el archivo
  archivo.close();

  return true;
}


const int blockSize = 8000; // Tamaño de bloque en bytes

String transcribeSpeech(const String &audioBase64, const char* apiKey) {
    HTTPClient http;

    // URL de la API de Google Cloud Speech-to-Text
    String url = "https://speech.googleapis.com/v1/speech:recognize?key=";
    url += apiKey;

    Serial.println(url);

    // Configurar la solicitud HTTP
    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    String transcribedText = "";

    int audioLength = audioBase64.length();
    int numBlocks = (audioLength + blockSize - 1) / blockSize; // Calcular el número de bloques

    for (int i = 0; i < numBlocks; ++i) {
        // Calcular el rango de bytes para este bloque
        int startPos = i * blockSize;
        int endPos = min((i + 1) * blockSize, audioLength);

        // Extraer el bloque de audio
        String audioBlock = audioBase64.substring(startPos, endPos);

        // Crear el cuerpo de la solicitud JSON para este bloque
        String jsonBody = "{\"config\": {\"encoding\":\"FLAC\",\"sampleRateHertz\":16000, \"languageCode\": \"es\"},\"audio\": {\"content\":\"";
        jsonBody += audioBlock;
        jsonBody += "\"}}";

        Serial.println(jsonBody);

        // Enviar la solicitud POST con el cuerpo JSON
        int httpResponseCode = http.POST(jsonBody);

        // Si la solicitud fue exitosa, obtener la transcripción
        if (httpResponseCode == HTTP_CODE_OK) {
            String response = http.getString();

            // Analizar la respuesta JSON para obtener la transcripción
            DynamicJsonDocument doc(4096); // Aumentar el tamaño del documento si es necesario
            DeserializationError error = deserializeJson(doc, response);

            if (error) {
                Serial.print("deserializeJson() failed: ");
                Serial.println(error.c_str());
            } else {
                if (doc.containsKey("results")) {
                    JsonObject result = doc["results"][0];
                    if (result.containsKey("alternatives")) {
                        JsonObject alternative = result["alternatives"][0];
                        if (alternative.containsKey("transcript")) {
                            transcribedText += alternative["transcript"].as<String>();
                        }
                    }
                } else {
                    Serial.println("No se encontró el campo 'results' en la respuesta JSON");
                }
            }
            Serial.println(response);
        } else {
            Serial.print("Error en la solicitud: ");
            Serial.println(httpResponseCode);
        }

        // Esperar un poco antes de enviar el siguiente bloque
        delay(100);
    }
    // Liberar los recursos
    http.end();

    return transcribedText;
}