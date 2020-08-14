#include <SoftwareSerial.h>

#define CHAR_DELAY_ESP8266 5

#define B_RX_PIN 2
#define B_TX_PIN 3
#define RELAY_PIN 4

SoftwareSerial esp8266 =  SoftwareSerial(B_RX_PIN, B_TX_PIN);

void setup() {
    pinMode(B_RX_PIN, INPUT);
    pinMode(B_TX_PIN, OUTPUT);
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, HIGH);
  
    Serial.begin(9600);
//    esp8266.begin(115200);
//    sendData("AT+UART_DEF=4800,8,1,0,0", 1000);
    esp8266.begin(4800);

    Serial.setTimeout(100);
    esp8266.setTimeout(100);
    
    Serial.println("Configuring ESP8266");
    
    sendData("AT+RST", 1000); // Reset
    sendData("AT+CWMODE=1", 1000); // Client mode
  
    delay(3000);
    String data = esp8266.readString();
    if (data.indexOf("CONNECTED") == -1) {
        Serial.println("Connecting to wi-fi");
        sendData("AT+CWJAP=\"ARIEL - 2.4GHz\",\"calabresa\"", 5000); // Conecta a rede wireless
    }

    sendData("AT+CIPSTA=\"192.168.15.201\",\"192.168.15.1\",\"255.255.255.0\"", 200); // Set ip
    sendData("AT+CIPMUX=1", 200); // Configura para multiplas conexoes
    sendData("AT+CIPSERVER=1,5555", 200); // Inicia o web server na porta 5555
  
    Serial.println("Finished!");
}

void loop() {
    if (esp8266.available()) {
        if (esp8266.find("+IPD,")) {
            delay(30);
            int connectionId = esp8266.read() - 48;
            
            String data = "HTTP/1.1 200 OK\nContent-Type: text/html\n\n";
            if (esp8266.find("GET /ON")) {
                data += "<h1>ON</h1>";
                digitalWrite(RELAY_PIN, HIGH);
                Serial.println("ON!");
            } else {
                data += "<h1>OFF</h1>";
                digitalWrite(RELAY_PIN, LOW);
                Serial.println("OFF!");
            }

            clearBufferEsp8266();

            sendData("AT+CIPSEND=" + String(connectionId) + "," + String(data.length()), 500);
            sendData(data, 500);

            sendData("AT+CIPCLOSE=" + String(connectionId), 500);
        }
    }
}

void sendData(String command, const int timeout) {
    for (int i = 0; i < command.length(); i++) {
        esp8266.print(command.charAt(i));
        delay(CHAR_DELAY_ESP8266);
    }
    esp8266.print('\r');
    esp8266.print('\n');

    boolean okFinded = false;
    char last;
    long int time = millis();
    while ((time + timeout) > millis()) {
        while (esp8266.available()) {
            char readed = esp8266.read();
            Serial.write(readed);

            if (last == 'O' && readed == 'K') {
                clearBufferEsp8266();
                return;
            }
            last = readed;
        }
    }
}

void clearBufferEsp8266() {
    delay(5);
    while (esp8266.available()) {
        delay(2);
        esp8266.read();
    }
}
