#include <Arduino.h>
#include <Ethernet.h>
#include <SPI.h>

// HTTP Settings
const byte MAC[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
const EthernetClient client;


// Office light
const char OFFICE_LIGHT_URL[] = "192.168.15.201";
const int OFFICE_LIGHT_PORT = 5555;


// Dontpad settings
const char DONTPAD_SERVER[] = "dontpad.com";
const char DONTPAD_UPDATE_STATE_URI[] = "/adba-house-controller/state";
const char DONTPAD_STATE_URI[] = "/adba-house-controller/state.body.json?lastUpdate=0";
const char DONTPAD_CLEAR_ACTIONS_URI[] = "/adba-house-controller/actions";
const char DONTPAD_ACTIONS_URI[] = "/adba-house-controller/actions.body.json?lastUpdate=0";


// Ports settings
const int BEDROOM_LIGHT_PORT = 3;


// Commands constants
const char OFFICE_LIGHT_COMMAND = 'A';
const char BEDROOM_LIGHT_COMMAND = 'B';
const char AIR_STATE_COMMAND = 'C';
const char AIR_TURBO_COMMAND = 'D';
const char AIR_TEMP_COMMAND = 'E';

const char STATE_OFF = '0';
const char STATE_ON = '1';


// States
char OFFICE_LIGHT_STATE = STATE_OFF;
char BEDROOM_LIGHT_STATE = STATE_OFF;
char AIR_STATE = STATE_OFF;
char AIR_TURBO = STATE_OFF;
char AIR_TEMP = char(21);


void setup() {
    Serial.begin(9600);
    
    Serial.println("Initializing Ports...");
    pinMode(BEDROOM_LIGHT_PORT, OUTPUT);
    digitalWrite(BEDROOM_LIGHT_PORT, LOW);

    Serial.println("Initializing Ethernet...");
    if (Ethernet.begin(MAC) == 0) {
        Serial.println("Failure to register to Ethernet with DHCP");
    } else {
        Serial.print("Connected ");
        Serial.println(Ethernet.localIP());
    }

    Serial.println("Retrieving state...");
    String body = doGET(DONTPAD_STATE_URI);
    processBody(body);
    sendAir();

    Serial.println("Initialized!");
}


void loop() {
    String body = doGET(DONTPAD_ACTIONS_URI);
    if (body.length() == 0) {
        Serial.println("Nothing to do!");

    } else {
        char airState = AIR_STATE;
        char airTurbo = AIR_TURBO;
        char airTemp = AIR_TEMP;

        processBody(body);
    
        if (airState != AIR_STATE || airTurbo != AIR_TURBO || airTemp != AIR_TEMP) {
            sendAir();
        }
        
        saveState();
        clearActions();
    }

    Serial.println("Waitting 2s...");
    delay(2000);
}


bool openConnection() {
    client.setTimeout(5000);
    if (!client.connect(DONTPAD_SERVER, 80)) {
        Serial.print("Failure to connect to ");
        Serial.print(DONTPAD_SERVER);
        Serial.println(":80");
        return false;
    }
    return true;
}


String doGET(char uri[]) {
    if (!openConnection()) {
        return "";
    }
  
    client.print("GET ");
    client.print(uri);
    client.println(" HTTP/1.1");

    populateHeaders();
    
    client.println();

    return getBody();
}


void doPOST(char uri[], char data[], int dataSize) {
    if (!openConnection()) {
        return "";
    }
  
    client.print("POST ");
    client.print(uri);
    client.println(" HTTP/1.1");

    populateHeaders();
    client.println("Content-Type: application/x-www-form-urlencoded;charset=UTF-8");

    client.print("Content-Length: ");
    client.println(dataSize);
    
    client.println();
    client.println(data);
    
    Serial.println("POST Response");
    byte buf[8];
    while (client.connected()) {
        int len = client.available();
        if (len > 0) {
            if (len > 8) {
                len = 8;
            }
            client.read(buf, len);
            Serial.write(buf, len);
        }
    }
    Serial.println("");
}


void populateHeaders() {
    client.print("Host: ");
    client.println(DONTPAD_SERVER);

    client.println("Connection: close");
}


String getBody() {
    client.find("\"body\":\"");

    String body = "";
    byte buf[8];
    while (client.connected()) {
        int len = client.available();
        if (len > 0) {
            if (len > 8) {
                len = 8;
            }
            client.read(buf, len);
            for (int i = 0; i < len; i++) {
                if (buf[i] == '"') {
                    client.stop();
                    return body;
                }
                body = body + char(buf[i]);
            }
        }
    }

    return body;
}


void saveState() {
    Serial.println("Saving state...");
    int bodySize = 27;
    char body[] = {'t', 'e', 'x', 't', '=',
            OFFICE_LIGHT_COMMAND, OFFICE_LIGHT_STATE, '%', '7', 'C',
            BEDROOM_LIGHT_COMMAND, BEDROOM_LIGHT_STATE, '%', '7', 'C',
            AIR_STATE_COMMAND, AIR_STATE, '%', '7', 'C',
            AIR_TURBO_COMMAND, AIR_TURBO, '%', '7', 'C',
            AIR_TEMP_COMMAND, (AIR_TEMP + 65)};
    doPOST(DONTPAD_UPDATE_STATE_URI, body, bodySize);
}


void clearActions() {
    Serial.println("Cleanning actions...");
    doPOST(DONTPAD_CLEAR_ACTIONS_URI, "", 0);
}


void processBody(String body) {
    Serial.println("Processing...");
    Serial.println(body);

    char key = 0;
    char value = 0;
    for (int i = 0; i < body.length(); i++) {
        if (body.charAt(i) == '|') {
            process(key, value);
            key = 0;
            value = 0;
        } else if (key == 0) {
            key = body.charAt(i);
        } else if (key > 0) {
            value = body.charAt(i);
        }
    }

    if (key > 0) {
        process(key, value);
    }
}


void process(char key, char value) {
    Serial.print("Processing \"");
    Serial.print(key);
    Serial.print("=");
    Serial.print(value);
    Serial.println("\"...");

    if (key == OFFICE_LIGHT_COMMAND) {
        OFFICE_LIGHT_STATE = (value == STATE_ON) ? STATE_ON : STATE_OFF;
        client.stop();
        client.setTimeout(2000);
        if (!client.connect(OFFICE_LIGHT_URL, OFFICE_LIGHT_PORT)) {
            Serial.println("Failure to connect to office-light");
        }
        client.print("GET /");
        client.println((value == STATE_ON) ? "ON" : "OFF");
        client.stop();
        
    } else if (key == BEDROOM_LIGHT_COMMAND) {
        BEDROOM_LIGHT_STATE = (value == STATE_ON) ? STATE_ON : STATE_OFF;
        digitalWrite(BEDROOM_LIGHT_PORT, (value == STATE_ON));
        
    } else if (key == AIR_STATE_COMMAND) {
        AIR_STATE = (value == STATE_ON) ? STATE_ON : STATE_OFF;
        
    } else if (key == AIR_TEMP_COMMAND) {
        AIR_TEMP = 65 - value;
        
    } else if (key == AIR_TURBO_COMMAND) {
        AIR_TURBO = (value == STATE_ON) ? STATE_ON : STATE_OFF;
        
    } else {
        Serial.print("Invalid key \"");
        Serial.print(key);
        Serial.println("\"");
    }
}


void sendAir() {
    Serial.print("Configuring AIR [state=");
    Serial.print(AIR_STATE == STATE_ON ? "ON" : "OFF");
    Serial.print(", temp=");
    Serial.print(AIR_TEMP);
    Serial.print(", turbo=");
    Serial.print(AIR_STATE == STATE_ON ? "ON" : "OFF");
    Serial.println("]");

    // TODO Implements...
}
