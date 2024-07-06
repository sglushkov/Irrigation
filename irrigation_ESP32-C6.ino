//Irrigation Controller NodeMCU Code by Stoyan Glushkov V1.3 (ESP32-C6)

#include <WebServer.h>
#include <WiFi.h>
#include <EEPROM.h>

//RTC Module library
#include <Wire.h>

//Define PCF8563 RTC modile I2C address
#define PCF8563address 0x51


//Define motor driver polarity change pins
#define SwitchPin1  10
#define SwitchPin2  11


//Define Time Variables for startup/end times
int beginhour1;
int beginminute1;
int beginhour2;
int beginminute2;


//define class Valve
class Valve {
public:
    String zone_name;
    int starthour1 = 0;
    int startminute1 = 0;
    int starthour2 = 0;
    int startminute2 = 0;
    int stophour1 = 0;
    int stopminute1 = 0;
    int stophour2 = 0;
    int stopminute2 = 0;
    int duration = 0;
    int zone_pin;
    bool last_state;
};


//Declare array of Valve objects
Valve valves[9];
const int arr_valves_size = 8;
const int Zone_Pins[8] = { 16,17,23,22,21,20,19,18 };


// Convert normal decimal numbers to binary coded decimal needed for RTC time
byte decToBcd(byte val) {
    return((val / 10 * 16) + (val % 10));
}


// Convert binary coded decimal to normal decimal numbers needed for RTC time
byte bcdToDec(byte val) {
    return((val / 16 * 10) + (val % 16));
}


int recalc_hours(int& inminutes){
  int hours=0;
  hours = (inminutes / 60);
  return hours;
}


int recalc_mins(int& inminutes, int& inhours){
  int minutes_remaining=0;
  minutes_remaining = inminutes - (inhours*60);
  return minutes_remaining;
}


//Initialize Valve objects and calculate times
void valves_init() {

  int remain_mins1 = 0;
  int add_hours1 = 0;
  int remain_mins2 = 0;
  int add_hours2 = 0;


//set first zone start times
Serial.println("Valves EEPROM reads");
    valves[0].starthour1 = EEPROM.read(8);
    valves[0].startminute1 = EEPROM.read(9);
    valves[0].starthour2 = EEPROM.read(10);
    valves[0].startminute2 = EEPROM.read(11);


//calc zones start times
Serial.println("Calculate start times");
//Serial.println("Zones count ");
//Serial.println(arr_valves_size);
    for (int i = 1; i <= arr_valves_size; ++i) {

        valves[i].startminute1 = valves[i-1].startminute1 + EEPROM.read(i-1);
        remain_mins1 = 0;
        
        if (valves[i].startminute1 >= 60){
          add_hours1 = recalc_hours(valves[i].startminute1);
          remain_mins1 = recalc_mins(valves[i].startminute1,add_hours1);
          valves[i].startminute1 = remain_mins1;
        }
        valves[i].starthour1 = valves[i-1].starthour1 + add_hours1;
        add_hours1 = 0;
 //       Serial.println(valves[i].startminute1);
 //       Serial.println(valves[i].starthour1);

//Serial.println("1st begin");


        valves[i].startminute2 = valves[i-1].startminute2 + EEPROM.read(i-1);
        remain_mins2 = 0;
        if (valves[i].startminute2 >= 60){
          add_hours2 = recalc_hours(valves[i].startminute2);
          remain_mins2 = recalc_mins(valves[i].startminute2,add_hours2);
          valves[i].startminute2 = remain_mins2;
        }
        valves[i].starthour2 = valves[i-1].starthour2 + add_hours2;
        add_hours2 = 0;

//Serial.println("2nd begin");

        while (valves[i].starthour1 >= 24) {
            valves[i].starthour1 = valves[i].starthour1 - 24;
        }

        while (valves[i].starthour2 >= 24) {
            valves[i].starthour2 = valves[i].starthour2 - 24;
        }
    }


//Serial.println("Calculate stop times");
//calculate zones stop times
    for (int i = 0; i < arr_valves_size ; ++i) {
        valves[i].stophour1 = valves[i + 1].starthour1;
        valves[i].stopminute1 = valves[i + 1].startminute1;
        valves[i].stophour2 = valves[i + 1].starthour2;
        valves[i].stopminute2 = valves[i + 1].startminute2;
    }


//set zone numbers
    for (int i = 0; i < arr_valves_size; ++i) {

        valves[i].zone_name = "Zone " + String(i + 1);

    }


 //set zone microcontroller pin
    for (int i = 0; i < arr_valves_size; ++i) {

        valves[i].zone_pin = Zone_Pins[i];

    }


 //get durations from EEPROM
//Serial.println("Get durations from EEPROM");
    for (int i = 0; i < arr_valves_size; ++i) {

        valves[i].duration = EEPROM.read(i);

    }
  //  Serial.println("Got durations from EEPROM");
}



int duration1;
int duration2;
int duration3;
int duration4;
int duration5;
int duration6;
int duration7;
int duration8;

// Clock set variables

int hourset = 0;
int minuteset = 0;
int secondset = 0;
int dayset = 0;

int hset_form;
int mset_form;
int sset_form;
int dset_form;

//Startup time varibales set by web form

int beginh1_form;
int beginmin1_form;
int beginh2_form;
int beginmin2_form;

int durat1_form;
int durat2_form;
int durat3_form;
int durat4_form;
int durat5_form;
int durat6_form;
int durat7_form;
int durat8_form;

int sys_sw_form;

int ondelay = 1000;
int offdelay = 1000;
int pwrdelay = 2000;
int start_off_delay = 500;
int swmargintime = 7;
int betweenswtime = 2;

String dayOfWeekHuman;
const String weekdays[7] = { "Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday" };

String rootpage;

String lastdisconnected = "no";
int lastcount = 0;

int system_sw;


//const char* ssid = "DB";
//const char* password = "bulsatcom";

const char* ssid = "DB";
const char* password = "bulsatcom";

IPAddress ip(192, 168, 88, 99);
IPAddress gateway(192, 168, 88, 65);
IPAddress subnet(255, 255, 255, 192);


WebServer server(80);


// Get RTC Time from PCF8563 module
void readPCF8563(byte* second,
    byte* minute,
    byte* hour,
    byte* dayOfWeek,
    byte* dayOfMonth,
    byte* month,
    byte* year) {
    Wire.beginTransmission(PCF8563address);
    Wire.write(0x02);
    Wire.endTransmission();
    Wire.requestFrom(PCF8563address, 7);
    *second = bcdToDec(Wire.read() & B01111111);
    *minute = bcdToDec(Wire.read() & B01111111);
    *hour = bcdToDec(Wire.read() & B00111111);
    *dayOfMonth = bcdToDec(Wire.read() & B00111111);
    *dayOfWeek = bcdToDec(Wire.read() & B00000111);
    *month = bcdToDec(Wire.read() & B00011111);
    *year = bcdToDec(Wire.read());



    int i = *dayOfWeek;
    dayOfWeekHuman = weekdays[i];

}


void TurnON() {
    digitalWrite(SwitchPin1, HIGH);
    digitalWrite(SwitchPin2, LOW);
}

void TurnOFF() {
    digitalWrite(SwitchPin1, LOW);
    digitalWrite(SwitchPin2, HIGH);
}

void TurnNONE() {
    digitalWrite(SwitchPin1, LOW);
    digitalWrite(SwitchPin2, LOW);
}


void SwitchON() {
    TurnON();
    delay(ondelay);
}

void SwitchOFF() {
    TurnOFF();
    delay(offdelay);
}

void SwitchNONE() {
    delay(pwrdelay);
    TurnNONE();
}


void TurnOFFonStartup() {
    SwitchOFF();

    for (int i = 0; i <= sizeof(Zone_Pins); ++i) {

        digitalWrite(Zone_Pins[i], HIGH);
        delay(start_off_delay);
        digitalWrite(Zone_Pins[i], LOW);

    }

    TurnNONE();

}

void RelaySwitch() {

    if (system_sw == 0) goto skipped;

    byte second, minute, hour, dayOfWeek, dayOfMonth, month, year; //declare time variables
    readPCF8563(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year); // get time variables from readPCF8563 function


    for (int i = 0; i < arr_valves_size; ++i) {
        if (valves[i].duration == 0) goto nextvalve;
        if (valves[i].last_state) goto switchoff;

        if (((hour == valves[i].starthour1) && (minute == valves[i].startminute1) &&
            (second > 0 && second < swmargintime)) ||
            (server.arg("sw" +String(i+1)+ "") == "on") ||
            ((hour == valves[i].starthour2) && (minute == valves[i].startminute2) &&
                (second > 0 && second < swmargintime))) {

            valves[i].last_state = true;
            SwitchON();
            digitalWrite(valves[i].zone_pin, HIGH);
            SwitchNONE();
            digitalWrite(valves[i].zone_pin, LOW);

        }

    switchoff:;

        if (!valves[i].last_state) goto nextvalve;

        if (((hour == valves[i].stophour1) && (minute == valves[i].stopminute1) &&
            (second > 0 && second < swmargintime)) ||
            (server.arg("sw" +String(i+1)+ "") == "off") ||
            ((hour == valves[i].stophour2) && (minute == valves[i].stopminute2) &&
                (second > 0 && second < swmargintime))) {

            valves[i].last_state = false;
            SwitchOFF();
            digitalWrite(valves[i].zone_pin, HIGH);
            SwitchNONE();
            digitalWrite(valves[i].zone_pin, LOW);
            delay(betweenswtime);
        }

    nextvalve:;

    }

skipped:;

}

void setup() {


// Initialize Pins
    Wire.begin(0, 1); // Sets - SDA (SD2 Pin), SCL (SD3 pin) For RTC module
   // EEPROM.begin(512);
    if (!EEPROM.begin(512)) {
    Serial.println("Failed to initialize EEPROM");
    return;
  }


//Set relay pins mode
    for (int i = 0; i <= sizeof(Zone_Pins); ++i) {

        pinMode(Zone_Pins[i], OUTPUT);

    }

//Set driver pins mode
    pinMode(SwitchPin1, OUTPUT);
    pinMode(SwitchPin2, OUTPUT);

    // Set outputs to LOW

    /*
      digitalWrite(SwitchPin1, LOW);
      digitalWrite(SwitchPin2, LOW);

      digitalWrite(Zone1Pin, LOW);
      digitalWrite(Zone2Pin, LOW);
      digitalWrite(Zone3Pin, LOW);
      digitalWrite(Zone4Pin, LOW);
      digitalWrite(Zone5Pin, LOW);
      digitalWrite(Zone6Pin, LOW);
      digitalWrite(Zone7Pin, LOW);
      digitalWrite(Zone8Pin, LOW);
      */

    TurnOFFonStartup();

    //duration1 = EEPROM.read(0);
    //duration2 = EEPROM.read(1);
    //duration3 = EEPROM.read(2);
    //duration4 = EEPROM.read(3);
    //duration5 = EEPROM.read(4);
    //duration6 = EEPROM.read(5);
    //duration7 = EEPROM.read(6);
    //duration8 = EEPROM.read(7);

    //beginhour1 = EEPROM.read(8);
    //beginminute1 = EEPROM.read(9);
    //beginhour2 = EEPROM.read(10);
    //beginminute2 = EEPROM.read(11);

    system_sw = EEPROM.read(12);


//Setup UART
    Serial.begin(115200);
    Serial.println("");
    Serial.println("START");

//Configure and start Wi-Fi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
        WiFi.config(ip, gateway, subnet);


//WiFi info serial log
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());


//Setup HTTP server
Serial.println("Server begin");
    server.begin();
    server.on("/", handleRoot);
    server.on("/sw1", handleSW);
    server.on("/sw2", handleSW);
    server.on("/sw3", handleSW);
    server.on("/sw4", handleSW);
    server.on("/sw5", handleSW);
    server.on("/sw6", handleSW);
    server.on("/sw7", handleSW);
    server.on("/sw8", handleSW);
    server.on("/update", HTTP_POST, []() {

   String beginmin1 = server.arg("bminute1");
   String beginh1 = server.arg("bhour1");
   String beginmin2 = server.arg("bminute2");
   String beginh2 = server.arg("bhour2");

   String durat1x = server.arg("durat1");
   String durat2x = server.arg("durat2");
   String durat3x = server.arg("durat3");
   String durat4x = server.arg("durat4");
   String durat5x = server.arg("durat5");
   String durat6x = server.arg("durat6");
   String durat7x = server.arg("durat7");
   String durat8x = server.arg("durat8");

   String sys_swx = server.arg("sys_sw");


 //Convert Strings from web form to integer

        beginmin1_form = beginmin1.toInt();
        beginh1_form = beginh1.toInt();
        beginmin2_form = beginmin2.toInt();
        beginh2_form = beginh2.toInt();

        durat1_form = durat1x.toInt();
        durat2_form = durat2x.toInt();
        durat3_form = durat3x.toInt();
        durat4_form = durat4x.toInt();
        durat5_form = durat5x.toInt();
        durat6_form = durat6x.toInt();
        durat7_form = durat7x.toInt();
        durat8_form = durat8x.toInt();


        valves[0].duration = durat1_form;
        valves[1].duration = durat2_form;

        sys_sw_form = sys_swx.toInt();

        if ((beginmin1_form != beginminute1) || (beginh1_form != beginhour1) || (beginmin2_form != beginminute2) || (beginh2_form != beginhour2)
            || (durat1_form != duration1) || (durat2_form != duration2) || (durat3_form != duration3) || (durat4_form != duration4)
            || (durat5_form != duration5) || (durat6_form != duration6) || (durat7_form != duration7) || (durat8_form != duration8) || (sys_sw_form != system_sw)) {
            SaveToEEPROM();
        }


        handleRoot();
        server.send(200, "text/html", rootpage);

        });

    server.on("/update_clock", HTTP_POST, []() {

        String hsetx = server.arg("hset");
        String msetx = server.arg("mset");
        String ssetx = server.arg("sset");
        String dsetx = server.arg("dset");

        hset_form = hsetx.toInt();
        mset_form = msetx.toInt();
        sset_form = ssetx.toInt();
        dset_form = dsetx.toInt();

        if (hset_form != 0 || mset_form != 0 || sset_form || sset_form != 0 || dset_form != 0) {

            setPCF8563();

            hset_form = 0;
            mset_form = 0;
            sset_form = 0;
            dset_form = 0;
        }

        Serial.println("Clock is Set");
        //      server.send(200, "text/plain", "Ok");
        handleRoot();
        server.send(200, "text/html", rootpage);

        });

    server.onNotFound(handleNotFound);
    server.begin();
      Serial.println("HTTP server started");

    // init valves
    valves_init();
    Serial.println("Valves initialized");

} //end of void setup


void setPCF8563() {
    byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;//declare time variables
    second = sset_form;
    minute = mset_form;
    hour = hset_form;
    dayOfWeek = dset_form;

    Wire.beginTransmission(PCF8563address);
    Wire.write(0x02);
    Wire.write(decToBcd(second));
    Wire.write(decToBcd(minute));
    Wire.write(decToBcd(hour));
    Wire.write(decToBcd(dayOfMonth));
    Wire.write(decToBcd(dayOfWeek));
    Wire.write(decToBcd(month));
    Wire.write(decToBcd(year));
    Wire.endTransmission();
}


void SaveToEEPROM() {

    beginhour1 = beginh1_form;
    beginminute1 = beginmin1_form;

    beginhour2 = beginh2_form;
    beginminute2 = beginmin2_form;

    duration1 = durat1_form;
    duration2 = durat2_form;
    duration3 = durat3_form;
    duration4 = durat4_form;
    duration5 = durat5_form;
    duration6 = durat6_form;
    duration7 = durat7_form;
    duration8 = durat8_form;

    system_sw = sys_sw_form;


    EEPROM.write(0, duration1);
    EEPROM.write(1, duration2);
    EEPROM.write(2, duration3);
    EEPROM.write(3, duration4);
    EEPROM.write(4, duration5);
    EEPROM.write(5, duration6);
    EEPROM.write(6, duration7);
    EEPROM.write(7, duration8);

    EEPROM.write(8, beginhour1);
    EEPROM.write(9, beginminute1);
    EEPROM.write(10, beginhour2);
    EEPROM.write(11, beginminute2);

    EEPROM.write(12, system_sw);


    EEPROM.commit();
    Serial.println("Saving to EEPROM");

    valves_init();


}

void checkwifi() {

    if (WiFi.status() != WL_CONNECTED) {

        lastdisconnected = "yes";
        lastcount = (lastcount + 1);
        WiFi.config(ip, gateway, subnet);
        WiFi.begin(ssid, password);
        WiFi.config(ip, gateway, subnet);
        delay(500);
        Serial.println(ssid);
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());

    }

}


//MAIN FUCTION LOOP

void loop() {

    RelaySwitch();
    server.handleClient();
    delay(500);
    checkwifi();
}

void  handleRoot() {

    //  server.send(200, "text/html", constructHtml());
    //server.send(200, "text/html", "OK");
    // }


    //String constructHtml(){
    byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;//declare time variables
    readPCF8563(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year); // get time variables from readPCF8563 function

    String ip = WiFi.localIP().toString();

    String html = "<!DOCTYPE html>\r\n";
    html += "<html>\r\n";
    html += "<head>\r\n";
    html += "<meta charset='UTF-8'>\r\n";
    html += "<font face='consolas'>\r\n";
    html += "<title>ESP32</title>\r\n";
    html += "<H1><body>\r\n";

    html += "<H1>ESP32-C6 Irrigation Controller v2.0</H1>";

    html += "<p><b>Current Time by Controller RTC module:</b> <br /> " + String(dayOfWeekHuman) + " - " + String(hour) + ":" + String(minute) + ":" + String(second) + "  <br />  </p>";
    html += "<br>";

    html += "<p>" + String(lastdisconnected) + " - " + String(lastcount) + " times</p>";
    html += "<br>";

    html += "<H1><a href=http://" + String(ip) + ">REFRESH</a></H1>";

    html += "<p><b>Timetable:</b></p>";

    html += "<table border=1>";
    html += "<tr>";
    html += "<td>Zone  </td>";
    html += "<td>ON/OFF</td>";
    html += "<td>Duration</td>";
    html += "<td>1st Start Time</td>";
    html += "<td>2nd Start Time</td>";
    html += "</tr>";

    for (int i = 0; i < arr_valves_size; ++i) {
        html += "<tr>";
        html += "<td>" + valves[i].zone_name + "</td>";
        html += "<td>";

        if (valves[i].last_state) {
            html += "<a href='sw" + String(i + 1) + "?sw" + String(i + 1) + "=off' style='display: inline-block; width: 100px; background-color: green; text-align: center;'>ON</a>\r\n";
        }
        else {
            html += "<a href='sw" + String(i + 1) + "?sw" + String(i + 1) + "=on' style='display: inline-block; width: 100px; background-color: red; text-align: center;'>OFF</a>\r\n";
        }
        html += "</td>";

        html += "<td>" + String(valves[i].duration) + "</td>";
        html += "<td>" + String(valves[i].starthour1) + ":" + String(valves[i].startminute1) + "</td>";
        html += "<td>" + String(valves[i].starthour2) + ":" + String(valves[i].startminute2) + "</td>";
        html += "</tr>";

    }


    html += "<tr>";
    html += "<td>Cycle End </td>";

    if (system_sw == 1) {
        html += "<td bgcolor='green'>System is ON</a>\r\n";
    }
    else {
        html += "<td bgcolor=red>System is OFF</a>\r\n";
    }
    html += "</td>";


    html += "<td></td>";
    html += "<td>" + String(valves[arr_valves_size - 1].stophour1) + ":" + String(valves[arr_valves_size - 1].stopminute1) + "</td>";
    html += "<td>" + String(valves[arr_valves_size - 1].stophour2) + ":" + String(valves[arr_valves_size - 1].stopminute2) + "</td>";
    html += "</tr>";
    html += "</table>";

    html += "<br>";

    html += "<p><b>Startup times and duration setup:</b></p>";

    html += "<p><b>Warnings:</b><br />  1. Do not set zone duration more than 255 minutes <br /></p>";

    html += "<form action='update' method='post'>";

    html += "<div>";
    html += "<label for='bhour1'>1st Begin Hour___</label>";
    html += "<input name='bhour1' id='bhour1' value=" + String(valves[0].starthour1) + ">";
    html += "</div>";
    html += "<div>";
    html += "<label for='bminute1'>1st Begin Minute_</label>";
    html += "<input name='bminute1' id='bminute1' value=" + String(valves[0].startminute1) + ">";
    html += "</div>";
    html += "<div>";
    html += "<label for='bhour2'>2nd Begin Hour___</label>";
    html += "<input name='bhour2' id='bhour2' value=" + String(valves[0].starthour2) + ">";
    html += "</div>";
    html += "<div>";
    html += "<label for='bminute2'>2nd Begin Minute_</label>";
    html += "<input name='bminute2' id='bminute2' value=" + String(valves[0].startminute2) + ">";

    html += "</div>";

    for (int i = 0; i < arr_valves_size; ++i) {

        html += "<div>";
        html += "<label for='durat" + String(i + 1) + "'>" + valves[i].zone_name + " Duration__</label>";
        html += "<input name='durat" + String(i + 1) + "' id='durat" + String(i + 1) + "' value=" + String(valves[i].duration) + ">";
        html += "</div>";

    }

    html += "<div>";
    html += "<label for='sys_sw'>System Status____</label>";
    html += "<input name='sys_sw' id='sys_sw' value=" + String(system_sw) + ">  1 for ON,  0 for OFF";
    html += "</div>";
    html += "<div>";
    html += "<input type='submit' value='Submit'>";
    html += "</div>";
    html += "</form>";



    // -------- Time Set-------------------------

    html += "<br>";
    html += "<br>";
    html += "<p><b>Clock set:</b></p>";

    html += "<form action='update_clock' method='post'>";

    html += "<div>";
    html += "<label for='hset'>Set Hour________</label>";
    html += "<input name='hset' id='hourset' value=" + String(hour) + ">";
    html += "</div>";
    html += "<div>";
    html += "<label for='mset'>Set Minute______</label>";
    html += "<input name='mset' id='munuteset' value=" + String(minute) + ">";
    html += "</div>";
    html += "<div>";
    html += "<label for='sset'>Set Seconds_____</label>";
    html += "<input name='sset' id='secondset' value=" + String(second) + ">";
    html += "</div>";
    html += "<div>";
    html += "<label for='dset'>Set Day of Week_</label>";
    html += "<input name='dset' id='dayset' value=" + String(dayOfWeek) + "> Insert Digit - 0 for Sunday";
    html += "</div>";
    html += "<div>";
    html += "<input type='submit' value='Set the Clock'>";
    html += "</div>";
    html += "</form>";


    html += "<br>";
    html += "</font>\r\n";
    html += "</body>\r\n</H1>";
    html += "</html>\r\n";

    rootpage = html;
    server.send(200, "text/html", html);

}

void handleSW() {

    /*  String html = "<!DOCTYPE html>\r\n";
      html += "<html>\r\n";
      html += "</html>\r\n";
    */

    String ip = WiFi.localIP().toString();
    server.sendHeader("Location", String("http://") + ip, true);

    //  server.send ( 302, "text/plain", "");

    RelaySwitch();
    handleRoot();
    server.send(200, "text/html", rootpage);

    //  server.client().stop();

}

void handleNotFound() {
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
    for (uint8_t i = 0; i < server.args(); i++) {
        message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    server.send(404, "text/plain", message);
}
