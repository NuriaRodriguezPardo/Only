# Transcripción de Audio desde SD y Conexión a WiFi

## Objetivo

El objetivo de este proyecto es capturar audio utilizando un ESP32-S3, almacenarlo en una tarjeta SD en formato Base64, conectarse a una red WiFi, y transcribir el audio utilizando la API de Google Cloud Speech-to-Text. El texto transcrito se imprime en el monitor serie.

## Materiales

- ESP32-S3
- Tarjeta SD y lector de tarjetas
- Librerias: 
```.ini
    lib_deps = 
        bblanchon/ArduinoJson@^7.0.4
        esphome/ESP32-audioI2S@^2.0.7
        earlephilhower/ESP8266Audio@^1.9.7
```

## Procedimiento

1. Conectar el ESP32-S3 a la tarjeta SD utilizando los pines adecuados.
2. Configurar los pines y la tarjeta SD en el código.
3. Conectar el ESP32-S3 a una red WiFi.
4. Leer el archivo de audio Base64 desde la tarjeta SD.
5. Enviar el audio a la API de Google Cloud Speech-to-Text para transcripción.
6. Imprimir el texto transcrito en el monitor serie.

### Código
```cpp
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
        File archivo = SD.open("/encoded_audio.txt", FILE_READ);
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
```

### Descripción
Este código configura el ESP32-S3 para leer un archivo de audio en formato Base64 desde una tarjeta SD, conectar el dispositivo a una red WiFi, y utilizar la API de Google Cloud Speech-to-Text para transcribir el audio. Primero, se definen las bibliotecas necesarias y las constantes para la configuración de WiFi y las APIs. La función ``connectToWiFi()`` establece la conexión WiFi. En ``setup()``, se inicializa la comunicación serie y se conecta a la red WiFi. Luego, se lee el archivo de audio Base64 desde la tarjeta SD utilizando ``obtenerAudioDesdeSD()``. La función ``getLanguageCode()`` se utiliza para obtener el código del idioma correspondiente al nombre del idioma. Finalmente, ``transcribeSpeech()`` envía el audio a la API de Google Cloud Speech-to-Text y obtiene la transcripción. El texto transcrito se imprime en el monitor serie.
### Diagrama de flujos
```mermaid
    graph TD
        A[Inicio] --> B[Conectar a WiFi]
        B --> C[Leer audio Base64 desde SD]
        C --> D[Obtener código de idioma]
        D --> E[Enviar audio a la API de Google Cloud Speech-to-Text]
        E --> F[Obtener transcripción de audio]
        F --> G[Imprimir texto transcrito en monitor serie]
        G --> H[Fin]
```
## Conclusión
Este proyecto muestra cómo conectar un ESP32-S3 a una red WiFi, leer un archivo de audio en formato Base64 desde una tarjeta SD, y utilizar la API de Google Cloud Speech-to-Text para transcribir el audio. La transcripción del audio se imprime en el monitor serie.