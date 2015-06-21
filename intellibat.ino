/*
	Intellibat 

	Smart battery monitor, designed for e-bikes and light electrical PWM or DC powered vehicles.

	Created april 2015
	By imayoda
	with solid contributions from:
        http://startingelectronics.org/
        http://allaboutee.com/
        and the gpl people @ 
        http://forum.arduino.cc/
        Cheers to all of you.
        
        Github
	https://github.com/imayoda/

        Requisites: Esp8266 with AT 0.9.5 firmware
                    Dht sensor
                    ACS712
*/

String softversion = "0.1 beta";  // Version
#include <TextFinder.h>  // add this library if you have not it yet
//#include <AltSoftSerial.h> // add this library and enable it if AltSoftSerial is in use
#define DEBUG true    // esp8266 command debug activation
#include <DHT.h>
#define DHTPIN 4    // what pin DTH is connected to 
#define DHTTYPE DHT11   // DHT 11 specified
DHT dht(DHTPIN, DHTTYPE, 10);

//TIMING
long interval = 60000; // for timed ADH reading
int interval1 = 1000; // for timed ACS712&voltage reading
int interval2 = 1000; // for timed Power&Uptime calculations
long int previousMillis = 0;     // for timed ADH reading
long int previousMillis1 = 0; // for timed ACS712 reading
long int previousMillis2 = 0;
//TEMPERATURE
float h = 0;
float t = 0;
//VOLTAGE
float pseudovolt = 0.0;
float averagevoltage = 0.0;
float realvoltage = 0.0;
float voltage = 0.0;    // calculated voltage
float percentvoltage = 0;
float realaveragevoltage = 0;
int counter = 0;
int counter1 = 0;
const int emptyvoltage = 45;  //Empty battery voltage
const int fullvoltage = 60;    //Full battery voltage
//UPTIME
 long days=0;
 long hours=0;
 long mins=0;
 long secs=0;
//CURRENT
const int AmpPin = A4;
const int Vref = 5.0;  // change to 3.3 if on 3.3V Arduino
const int Factor = 66; //185, 100, or 66 depending on ACS maximum current model in use 5A, 20A or 30A (AKA sensor sensitivity)
const int numcycle = 10;
float current = 0.0;
float realcurrent = 0.0;
//POWER
int fullbattamps = 5; // Battery capacity in Amperes
float averagecurrent = 0.0;
int realaveragecurrent = 0.0;
int counter2 = 0;
float instantpower = 0;
float realaveragepower = 0;
float timetoempty = 0.0;

//SERIAL ALTERNATIVES #use none if you have Hardware Serials available (Leonardo, Teensy, Mega etc.)
//change every Serial1. in the sketch to esp8266 if you want
//to use one of the two software serials (btw AltSoftSerial is better choice)

//esp8266 related
//AltSoftSerial esp8266;        // make RX Arduino line is pin 8, make TX Arduino line is pin 9.
                              // This means that you need to connect the TX line from the esp to the Arduino\"s pin 2
                              // and the RX line from the esp to the Arduino\"s pin 3
//SoftwareSerial esp8266(8,9);// make RX Arduino line is pin 2, make TX Arduino line is pin 3.
                              // This means that you need to connect the TX line from the esp to the Arduino\"s pin 2
                              // and the RX line from the esp to the Arduino\"s pin 3
////IP gain
String ip;

          
void setup()
{
  delay(1000);
  Serial.begin(57600);
  Serial1.begin(57600); // your esp's baud rate might be different
  delay(1000);
  sendData("AT+RST\r\n",2000,DEBUG); // reset module for a clean start
  delay(1000);
  TextFinder finder(Serial1);
 dht.begin(); // DHT begins
 delay(1000); //give time to DHT for warm up
 Serial.println(F("Starting."));
 delay(3000);
 Serial.println(F("Starting.."));  // Give time for ESP8266 to connect to an eventual slow router
 delay(3000);
 Serial.println(F("Started..."));
 delay(3000);
 //######### FIRST ACTIVATION COMMAND CLUSTER ###########
  //sendData("AT+CIOBAUD=57600\r\n",2000,DEBUG);                                                                                      // This parameters are only needed
  //sendData("AT+CWMODE=3\r\n",2000,DEBUG); // configure as access point (1 client,2 ap,3 mixed)                                      // during the first execution
  //sendData("AT+CWSAP=\"essid\",\"password\",9,3\r\n",10000,DEBUG); // soft access point credentials                                 // just to set up AP+STA mode, credentials
  //sendData("AT+CWJAP=\"essid\",\"password\"\r\n",10000,DEBUG); // set access point credentials to join already active AccessPoint   // and client to existing AccessPoint

  sendData("AT+CIFSR\r\n",2000,DEBUG); // get ip address                              // Only this params are actually needed
  sendData("AT+CIPMUX=1\r\n",1000,DEBUG); // configure for multiple connections       // once the connections parameters have 
  sendData("AT+CIPSERVER=1,80\r\n",1000,DEBUG); // turn on server on port 80          // loaded first time and are working
  sendData("AT+CIPSTO=15\r\n",1000,DEBUG);                                            // correctly for your environment.

 Serial.println(F(".............."));
Serial.println(F("ESP8266 is ONLINE and waiting for connections."));


}

void loop()
{ 
  
//################################ LATE CALL FOR ESP8266 and WEB PAGE ########################################



  
  if(Serial1.available()) // check if the esp is sending a message 
  
  
  {
  if(Serial1.find("+IPD,"))

    {
      
                   int connectionId = Serial1.read()-48; // subtract 48 because the read() function returns 
                                   // the ASCII decimal value and 0 (the first decimal number) starts at 48
                    if (connectionId == -49){
                      String closeCommand = "AT+CIPCLOSE="; 
                           closeCommand+=connectionId; // append connection id
                           closeCommand+="\r\n";
                           sendData(closeCommand,1500,DEBUG);
                           sendData("AT+CIPMUX=0\r\n",1000,DEBUG); // configure for multiple connections
                           delay(500);
                           sendData("AT+CIPMUX=1\r\n",1000,DEBUG); // configure for multiple connections
                         }
                    
                    else {
                    Serial.println(F("connectionId is: "));
                    Serial.print(connectionId);                                      

                    
                    if (Serial1.find("*ajax*")) { //// AJAX request has arrived ?(base page has loaded and is asking for div data ?) /////
                      
                           
                           String webpage = F("<p>Uptime: ");
                           webpage += String(hours);
                           webpage += F(":");
                           webpage += String(mins);
                           webpage += F(":");
                           webpage += String(secs);
                           webpage += F("</p>");
                           webpage += F("<h1>Statistiche elettriche</h1><p>Voltaggio batteria: ");
                           webpage += String(realaveragevoltage,1);
                           webpage += F(" V</p><p>Percentuale stimata: ");
                           if (percentvoltage >= 52){                  
                           webpage += F("<font color='green'>");
                           webpage += String(percentvoltage,0);
                           webpage += F("</font>");
                           }
                           else {
                           webpage += F("<font color='red'>");
                           webpage += String(percentvoltage,0);          
                           webpage += F("</font>");
                           }
                           webpage += F(" %</p><p>Consumo: ");
                           webpage += String(realcurrent,1);
                           webpage += F(" A</p><h2>Stime:</h2>");
                           webpage += F("<p>Potenza instantanea: ");
                           webpage += String(instantpower,0);
                           webpage += F(" W</p><p>Potenza media: ");
                           webpage += String(realaveragepower,0);
                           webpage += F(" W</p>");
                           if (timetoempty >= 0) {
                           webpage += F("<p>Tempo residuo batt.: ");
                           webpage += String(timetoempty,0);
                           }
                           else {
                           webpage += F("<p>Tempo ricarica batt.: ");
                           webpage += String(timetoempty,0 * -1);
                           }
                           webpage += F(" min.</p><h2>Dati ambientali</h2><p>Temperatura= ");
                           webpage += String(t, 1);
                           webpage += F(" C*</p><p>Umidita= ");
                           webpage += String(h, 1);
                           webpage += F(" %</p>");
                           
                           String cipSend = "AT+CIPSEND=";
                           cipSend += connectionId;
                           cipSend += ",";
                           cipSend += webpage.length();
                           cipSend +="\r\n";       
                           sendData(cipSend,350,DEBUG);
                           sendData(webpage,350,DEBUG);
                           
                           webpage = F("<p>IP= ");
                           webpage += String(ip);                        
                           webpage += F("</p>");
                           
                           cipSend = "AT+CIPSEND=";
                           cipSend += connectionId;
                           cipSend += ",";
                           cipSend += webpage.length();
                           cipSend +="\r\n";       
                           sendData(cipSend,350,DEBUG);
                           sendData(webpage,350,DEBUG);
                           
                          
                           //####### END AJAX CONNECTION ######################
                           String closeCommand = "AT+CIPCLOSE="; 
                           closeCommand+=connectionId; // append connection id
                           closeCommand+="\r\n";
                           sendData(closeCommand,1000,DEBUG);
                    
                    }
               else {     
        
                 
       String webpagescript = F("<html><title>Intellibat</title><script>function GetAjax() {var request = new XMLHttpRequest(); request.onreadystatechange = function() { if(this.readyState === 4){ if(this.status === 200){ if(this.responseText !== null){document.getElementById('data').innerHTML = this.responseText; } } } };  request.open('GET', '*ajax*', true); request.send(null);} var timer=self.setInterval(GetAjax, 60000);</script>"); 
              
             String cipSend = "AT+CIPSEND=";
     cipSend += connectionId;
     cipSend += ",";
     cipSend += webpagescript.length();
     cipSend +="\r\n";       
     sendData(cipSend,350,DEBUG);
     sendData(webpagescript,350,DEBUG);           


              webpagescript = F("<body><div id='data' onclick='GetAjax()' style='border-radius:25px;background:#EBFFAD;padding:20px;width:480px;height:550px;border:4px solid green;border-style:outset;'><h2>Intellibat by imayoda</h2><br/><a style='text-decoration: blink;'>Loading</a></div><div style='border-radius:25px;background:#CCFFFF;padding:20px;width:480px;height:50px;border:4px solid green;border-style:outset;'>");
     cipSend = "AT+CIPSEND=";
     cipSend += connectionId;
     cipSend += ",";
     cipSend += webpagescript.length();
     cipSend +="\r\n";       
     sendData(cipSend,350,DEBUG);
     sendData(webpagescript,350,DEBUG);    
     
     
              webpagescript = F("<div style='width:240px;height:50px;float:left;padding-top:10px;'><input type='checkbox' onclick='if(this.checked){clearInterval(timer); timer=setInterval(GetAjax, 10000);}else{clearInterval(timer); timer=setInterval(GetAjax, 60000);}' id='myCheckbox' style='transform:scale(4, 4);' /><label for='myCheckbox' style='font-size:150%;padding-left:20px;'>Refresh Veloce</label></div>");
     cipSend = "AT+CIPSEND=";
     cipSend += connectionId;
     cipSend += ",";
     cipSend += webpagescript.length();
     cipSend +="\r\n";       
     sendData(cipSend,350,DEBUG);
     sendData(webpagescript,350,DEBUG);
  
  
              webpagescript = F("<div style='width:240px;height:50px;float:right;'><p align='right'>");
              webpagescript += String(softversion);
              webpagescript += F("<br/></p></div></div></body></html>");
     cipSend = "AT+CIPSEND=";
     cipSend += connectionId;
     cipSend += ",";
     cipSend += webpagescript.length();
     cipSend +="\r\n";       
     sendData(cipSend,350,DEBUG);
     sendData(webpagescript,350,DEBUG);
      if (esp8266.find("busy..")){
      software_Reset();
      }

//####### END CONNECTION ######################
     String closeCommand = "AT+CIPCLOSE="; 
     closeCommand+=connectionId; // append connection id
     closeCommand+="\r\n";
     sendData(closeCommand,2000,DEBUG);
     
     getIP();
     }
    } 
   }
  }

delay(500);

//////////////////// CALCULATIONS///////////////////////////////////////////


long int currentMillis = millis();  
 
  if(currentMillis - previousMillis > interval) {
    previousMillis = currentMillis;
    getTemp();
    }
    
long int currentMillis1 = millis();

  if (currentMillis1 - previousMillis1 > interval1) {
    previousMillis1 = currentMillis1;    
    getVolts();
    getAmps();
    }
    
    
long int currentMillis2 = millis();
  if (currentMillis2 - previousMillis2 > interval2) {
    getTime();
    getPower();
    }


delay(100);
}


  
String sendData(String command, const int timeout, boolean debug)
{   
Serial1.flush();
  
    String response = "";
    
    Serial1.print(command); // send the read character to the esp8266
    long int time = millis();
    
    while( (time+timeout+120) > millis())
    {
      while(Serial1.available())
      {
        
        // The esp has data so display its output to the serial window 
        char c = Serial1.read(); // read the next character.
        response+=c;
      }  
    }
    
    if(debug)
    {
      Serial.print(response);
    }
    
    return response;
    
}
void software_Reset() // just there for emergency use
{
  asm volatile ("  jmp 0");  
} 


void getTime()
{
  //############################################ UPTIME  #################################

 secs = millis()/1000; //convect milliseconds to seconds
 mins=secs/60; //convert seconds to minutes
 hours=mins/60; //convert minutes to hours
 days=hours/24; //convert hours to days
 secs=secs-(mins*60); //subtract the coverted seconds to minutes in order to display 59 secs max 
 mins=mins-(hours*60); //subtract the coverted minutes to hours in order to display 59 minutes max
 hours=hours-(days*24); //subtract the coverted hours to days in order to display 23 hours max

//############################################ UPTIME ^^^^  #################################
}

void getTemp()
{
  //####################################### TEMPERATURE and HUMIDITY #####################
     
     // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds \"old\" (its a very slow sensor) AND IT STILL IS..
    h = dht.readHumidity();
    t = dht.readTemperature();
    
  }

//########################################### POWER CALCULATIONS ############################

void getVolts()
{
  //####### VOLTS


    pseudovolt = analogRead(A2)+1;
    voltage = (pseudovolt * Vref) / 1024.0; // 5V powered arduino, change in initial setup if needed to 3.3V if on arduino low voltage
    realvoltage = (voltage * 22); // voltage calibration parameter <-------- you may want to edit it
    
  counter1++;
  averagevoltage += (float)realvoltage; 

  
  if (counter1 > 10){
    counter1 = 0;
    averagevoltage = 0;
    }
  if (counter1 == 10) {
    realaveragevoltage = (float)averagevoltage / counter1;
    
    }    
    
percentvoltage = map(realaveragevoltage, emptyvoltage, fullvoltage, 0,100);  // 0 - 100% voltage indication (linear and negative/over 101% support)
}


void getAmps()
{
  //####### AMPERES

  current = analogRead(AmpPin); 
  if (current >= 509 && current <= 514){ // needed for correct 0 point reading
    current = 512;
    }
  realcurrent = (((current - 512) * Vref * 1000.0) / 1024) / Factor;
}


void getPower()
{
  //####### POWER

  counter2++;
  averagecurrent += (float)realcurrent; // all this is 1 minute current average calculation
  
  if (counter2 > 31){
    counter2 = 0;
    averagecurrent = 0;
    }
  if (counter2 == 31) {
    realaveragecurrent = (float)averagecurrent / counter2;
    }
instantpower = (float)realvoltage * (float)realcurrent; // instant power drain in W
realaveragepower = (float)realaveragevoltage * (float)realaveragecurrent; // average power drain in W half minute wise
timetoempty =  abs((((fullbattamps * realaveragevoltage) * (percentvoltage) / 100) / realaveragepower) * 60);// minutes to empty battery estimation

}

/////////////// GET IP
// reads assigned IP from router or smartphone's tether
// maximum support for 10 numbers IP adresses 
// (eg. 192.162.xx.xx -> supported; 192.168.xxx.x -> supported; 192.168.xxx.xx -> will not display last number;) 
void getIP() {

  ip = "";
  delay(50);
  ip = String(sendData("AT+CIFSR\r\n",1000,DEBUG));
  ip.remove(0, 85);
  ip.remove(13);
  ip.replace("\"", " ");

}

