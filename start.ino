#include <ESP8266WiFi.h>

// Определяет тип, используемый для хранения целочисленных значений в JsonVariant. 
// 0 - Long
// 1 - Long Long
#define ARDUINOJSON_USE_LONG_LONG 1

#include <ArduinoJson.h>

#include<AccelStepper.h>

AccelStepper myStepper(8, 13, 14, 12, 16);

// Количество шагов для оборота шагового двигателя
const int stepsPerRevolution = 4076;

// Настройка Wi-Fi
const char *ssid = "";
const char *password = "";

const char *balanceJsonField = "balance";

// Адрес кубсат ноды МИР
const char *host = "192.168.0.100";
const int port = 80;
const String address = "Address Wallet";

// Стоимость действия
const unsigned long long price = 100000000;

DynamicJsonBuffer jsonBuffer;
unsigned long long balance = -1;
unsigned int candiesToGive = 0;

void connectToWiFi(){
    Serial.printf("Connecting to %s ", ssid);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println(" connected");

}

void setup() {
    // Устанавливает скорость передачи данных в битах в секунду (бод) для последовательной передачи данных.
    // Для связи с компьютером используйте 115200
    Serial.begin(115200);
    Serial.println();

    // Желаемая постоянная скорость в шагах в секунду. Положительное значение по часовой стрелке.
    // Скорости более 1000 шагов в секунду ненадежны.
    myStepper.setMaxSpeed(700.0);
    myStepper.setAcceleration(700.0);
    myStepper.setSpeed(700);
    myStepper.moveTo(myStepper.currentPosition() - stepsPerRevolution / 2);

    if (ssid == ""){
        Serial.println("WiFi ssid is empty.");
    }

    connectToWiFi();
}


void loop() {

    // Проверяет WiFi и переподключает при необходимости
    if (WiFi.status() != WL_CONNECTED){
        WiFi.disconnect();
        connectToWiFi();
    }

    if (candiesToGive > 0) {
        if (myStepper.distanceToGo() == 0) {
            candiesToGive--;
            Serial.printf("[Motor was given, %d left]\n", candiesToGive);
            myStepper.moveTo(myStepper.currentPosition() - stepsPerRevolution / 2);
        }
        myStepper.run();
    } else {
        // Устройство сделает HTTP-запросы и получит ответ JSON
        WiFiClient client;

        Serial.printf("\n[Connecting to %s ... ", host);
        if (client.connect(host, port)) {
            Serial.println("connected]");

            Serial.println("[Sending a request]");
            client.print(String("GET /addresses/balance/" + address) + " HTTP/1.1\r\n" +
                         "Host: " + host + "\r\n" +
                         "Connection: close\r\n" +
                         "\r\n"
            );

            Serial.println("[Response:]");
            while (client.connected()) {
                if (client.available()) {
                    // Parse JSON
                    JsonObject &root = jsonBuffer.parse(client);
                    if (root.containsKey(balanceJsonField)) {
                        root.printTo(Serial);
                        const unsigned long long newBalance = root[balanceJsonField];
                        Serial.print("\nUpdated balance: ");
                        printLLNumber(newBalance, 10);
                        if (balance != -1) {
                            int candies = (newBalance - balance) / price;
                            candiesToGive += candies;
                        }
                        balance = newBalance;
                    }
                }
            }
            client.stop();
            Serial.println("\n[Disconnected]");
        } else {
            Serial.println("connection failed!]");
            client.stop();
        }
        // Подождите 5 секунд перед следующим действием
        delay(5000);
    }
}

// Функция для печати long long чисел
void printLLNumber(unsigned long long n, uint8_t base) {
    unsigned char buf[16 * sizeof(long)]; // Предполагает 8-битные символы.
    unsigned long long i = 0;

    if (n == 0) {
        Serial.print('0');
        return;
    }

    while (n > 0) {
        buf[i++] = n % base;
        n /= base;
    }

    for (; i > 0; i--)
        Serial.print((char) (buf[i - 1] < 10 ?
                             '0' + buf[i - 1] :
                             'A' + buf[i - 1] - 10));
}
