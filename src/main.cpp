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

bool obtenerAudioDesdeSD(char* &audioBase64);
String getLanguageCode(const char* languageName);
const char* transcribeSpeech(char* audioBase64, const char* apiKey, String idioma_target);

const char* ssid = "RedmiNuria";
const char* password = "Patata123";
const char* apiKey = "AIzaSyCVmhPyvDX0R8hTu6W8b4gBVqsRVIMYZOI";
const char* apiKey2 = "AIzaSyCz4Pb-7OIi3Gs6LGgJ-XHZ2Xy__hRAeZQ";


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

    char* audioBase;

    if (obtenerAudioDesdeSD(audioBase)) {
      Serial.println("Archivo de audio leído correctamente");
      // String audioBase64 = readAudioFileAndConvertToBase64(filename);
      // Convertir el audio a texto
      const char* idioma = "Spanish"; 
      String idioma_target = getLanguageCode(idioma); 
      const char* transcribedText = transcribeSpeech(audioBase, apiKey, idioma_target);
      // Imprimir el texto transcrito
      Serial.println("Texto transcrito:");
      Serial.println(transcribedText);
      // Aquí puedes procesar el buffer de audio
    } else {
        Serial.println("Error al leer el archivo de audio");
    }
}

const int chipSelect = 39; // Cambia esto según el pin que uses

bool obtenerAudioDesdeSD(char* &audioBase64) {
    // Inicializar la tarjeta SD
    SPI.begin(36, 37, 35); // void begin(int8_t sck=-1, int8_t miso=-1, int8_t mosi=-1, int8_t ss=-1);
    if (!SD.begin(chipSelect)) {
        Serial.println("Error al montar la tarjeta SD");
        return false;
    }

    // Abrir el archivo de texto con el contenido Base64
    File archivo = SD.open("/audio_malo.txt", FILE_READ);
    if (!archivo) {
        Serial.println("Error al abrir el archivo de texto");
        return false;
    }

    // Leer el contenido del archivo de texto y almacenar en un buffer dinámico
    int fileSize = archivo.size();
    audioBase64 = (char*)malloc(fileSize + 1);
    if (audioBase64 == nullptr) {
        Serial.println("Error al asignar memoria");
        archivo.close();
        return false;
    }

    int index = 0;
    while (archivo.available()) {
        char c = archivo.read();
        // Solo agregar caracteres válidos de Base64
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '+' || c == '/' || c == '=') {
            audioBase64[index++] = c;
        }
    }
    audioBase64[index] = '\0'; // Asegurar que la cadena está terminada en null

    // Cerrar el archivo
    archivo.close();
    Serial.println(audioBase64);
    return true;
}


String getLanguageCode(const char* languageName) {
    HTTPClient http;
    String url = "https://translation.googleapis.com/language/translate/v2/languages?key=";
    url += apiKey2;
    url += "&target=en";  // Para obtener los nombres de idiomas en inglés

    http.begin(url);
    int httpResponseCode = http.GET();


    String languageCode = "";
    if (httpResponseCode == HTTP_CODE_OK) {
        String response = http.getString();
        StaticJsonDocument<4096> doc;
        deserializeJson(doc, response);
        //DynamicJsonDocument doc(4096);
        //deserializeJson(doc, response);

        JsonArray languages = doc["data"]["languages"];
        for (JsonObject language : languages) {
            if (String(language["name"].as<const char*>()).equalsIgnoreCase(languageName)) {
                languageCode = language["language"].as<String>();
                break;
            }
        }
    } else {
        Serial.print("Error en la solicitud: ");
        Serial.println(httpResponseCode);
    }

    http.end();
    return languageCode;
}

const char* transcribeSpeech(char* audioBase64, const char* apiKey, String idioma_target) {
    HTTPClient http;

    // URL de la API de Google Cloud Speech-to-Text
    String url = "https://speech.googleapis.com/v1/speech:recognize?key=";
    url += apiKey;

    Serial.println(url);

    // Configurar la solicitud HTTP
    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    String transcribedText;

    // Crear el cuerpo de la solicitud JSON
    String jsonBody = "{\"config\": {\"encoding\":\"MP3\",\"sampleRateHertz\":16000, \"languageCode\": \"";
    jsonBody += idioma_target;
    jsonBody += "\"},\"audio\": {\"content\":\"";
    jsonBody += audioBase64;
    jsonBody += "\"}}";
    
    free(audioBase64);
    audioBase64 = nullptr;

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

    // Liberar los recursos
    http.end();

    // Asignar memoria para el texto transcrito y copiar la cadena
    char* transcriptionBuffer = (char*)malloc(transcribedText.length() + 1);
    if (transcriptionBuffer) {
        strcpy(transcriptionBuffer, transcribedText.c_str());
    }

    return transcriptionBuffer;
}


void loop() {
  // put your main code here, to run repeatedly:
}