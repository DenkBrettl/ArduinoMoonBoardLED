#include <HardwareBLESerial.h>
#include <NeoPixelBus.h>
#include <config.h>

HardwareBLESerial &bleSerial = HardwareBLESerial::getInstance();

#ifdef GRB
NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(PixelCount, PixelPin);
#else
NeoPixelBus<NeoRgbFeature, Neo800KbpsMethod> strip(PixelCount, PixelPin);
#endif

RgbColor red(BRIGHTNESS, 0, 0);
RgbColor green(0, BRIGHTNESS, 0);
RgbColor blue(0, 0, BRIGHTNESS);
RgbColor yellow(ADDITIONAL_LED_BRIGHTNESS, ADDITIONAL_LED_BRIGHTNESS, 0);
RgbColor cyan(0, BRIGHTNESS, BRIGHTNESS);
RgbColor pink(BRIGHTNESS, 0, BRIGHTNESS/2);
RgbColor violet(BRIGHTNESS/2, 0, BRIGHTNESS);
RgbColor black(0);

int state = 0; // Variable to store the current state of the problem string parser
String problemstring = ""; // Variable to store the current problem string
bool useadditionalled = false; // Variable to store the additional LED setting
int problem_loaded_time = 0; // Variable to store the time when a problem was loaded

void setup() {
  Serial.begin(9600);

  if (!bleSerial.beginAndSetupBLE(BLUETOOTH_NAME)) { // Initialize BLE UART and check if it is successful
    // This should never happen as it means that the BLE setup failed and the program cannot run!
    while (true) {
      Serial.println("BLE setup failed!");
      delay(1000);
    }
  }

  strip.Begin(); // Initialize LED strip
  strip.Show(); // Good practice to call Show() in order to clear all LEDs

  // Test LEDs by cycling through the colors and then turning the LEDs off again
  strip.SetPixelColor(0, green);
  for (int i = 0; i < PixelCount; i++) {
    strip.ShiftRight(1);
    strip.Show();
    delay(10);
  }
  strip.SetPixelColor(0, blue);
  for (int i = 0; i < PixelCount; i++) {
    strip.ShiftRight(1);
    strip.Show();
    delay(10);
  }
  strip.SetPixelColor(0, yellow);
  for (int i = 0; i < PixelCount; i++) {
    strip.ShiftRight(1);
    strip.Show();
    delay(10);
  }
  strip.SetPixelColor(0, cyan);
  for (int i = 0; i < PixelCount; i++) {
    strip.ShiftRight(1);
    strip.Show();
    delay(10);
  }
  strip.SetPixelColor(0, pink);
  for (int i = 0; i < PixelCount; i++) {
    strip.ShiftRight(1);
    strip.Show();
    delay(10);
  }
  strip.SetPixelColor(0, violet);
  for (int i = 0; i < PixelCount; i++) {
    strip.ShiftRight(1);
    strip.Show();
    delay(10);
  }
  strip.SetPixelColor(0, red);
  for (int i = 0; i < PixelCount; i++) {
    strip.ShiftRight(1);
    strip.Show();
    delay(10);
  }

  strip.ClearTo(black);
  strip.Show();

  // Wait for the MoonBoard App to connect
  while (!bleSerial);
}

void loop() {
  // Read messages from the BLE UART
  bleSerial.poll();
  
  // Check if message parts are available on the BLE UART
  while (bleSerial.available() > 0) {
    char c = bleSerial.read();

    // State 0: wait for configuration instructions (sent only if V2 option is enabled in app) or beginning of problem string
    if (state == 0) {
      if (c == '~') {
        state = 1; // Switch to state 1 (read configuration)
        continue;
      }
      if (c == 'l') {
        state = 2; // Switch to state 2 (read problem string)
        continue;
      }
    }

    // State 1: read configuration option (sent only if V2 option is enabled in app)
    if (state == 1) { 
      if (c == 'D') {
        useadditionalled = true;
        state = 2; // Switch to state 2 (read problem string)
        continue;
      }
      if (c == 'l') {
        state = 2; // Switch to state 2 (read problem string)
        continue;        
      }
    }

    // State 2: wait for the second part of the beginning of a new problem string (# after the lower-case L)
    if (state == 2) {
      if (c == '#') {
        state = 3; // Switch to state 3
        continue;
      }
    }

    // State 3: store hold descriptions in problem string
    if (state == 3) { 
      if (c == '#') { // problem string ends with #
        state = 4; // Switch to state 4 (start parsing and show LEDs)
        continue;
      }
      problemstring.concat(c); // add current character to problem string
    }
  }

  // State 4: complete problem string received, start parsing
  if (state == 4) {
    strip.ClearTo(black); // Turn off all LEDs in LED string
    Serial.println("\n---------");
    Serial.print("Problem string: ");
    Serial.println(problemstring);
    Serial.println("");

    String problemstringstore = problemstring; // store copy of problem string

    if (useadditionalled) { // only render additional LEDs in first loop
      Serial.println("Additional LEDs:");
      while(true){
        int pos = problemstring.indexOf(','); // Hold descriptions are separated by a comma (,)
        
        String hold;
        if (pos > 0) { // Still holds left in the problem string
          hold = problemstring.substring(0, pos); // Extract one hold description
        }
        else { // Last hold in the problem string
          hold = problemstring; 
        }

        char holdtype = hold.charAt(0); // Hold descriptions consist of a hold type (S, P, E) ...
        int holdnumber = hold.substring(1).toInt(); // ... and a hold number
        int lednumber = ledmapping[holdnumber];
        int additionallednumber = lednumber + additionalledmapping[holdnumber];
        if (additionalledmapping[holdnumber] != 0) {
          Serial.print(holdtype);
          Serial.print(holdnumber);
          Serial.print(" --> ");
          Serial.print(additionallednumber);
          if (holdtype == 'S') { // Start hold
            strip.SetPixelColor(additionallednumber, yellow);
            Serial.println(" (yellow)");
          }
          // Right, left, match, or foot hold
          if (holdtype == 'R' || holdtype == 'L' || holdtype == 'M' || holdtype =='F' || holdtype =='P') {
            strip.SetPixelColor(additionallednumber, yellow);
            Serial.println(" (yellow)");
          }
          // Finish holds don't get an additional LED!
        }        

        if (pos == -1) { // Last hold has been processed!
          Serial.println("");
          break;
        }

        problemstring = problemstring.substring(pos+1, problemstring.length()); // Remove processed hold from string
      }

      problemstring = problemstringstore; // Restore problem string for rendering normal hold LEDs
    }

    Serial.println("Problem LEDs:");
    while(true){ // render all normal LEDs (possibly overriding additional LEDs)
      int pos = problemstring.indexOf(','); // Hold descriptions are separated by a comma (,)
      
      String hold;
      if (pos > 0) { // Still holds left in the problem string
        hold = problemstring.substring(0, pos); // Extract one hold description
      }
      else { // Last hold in the problem string
        hold = problemstring; 
      }

      char holdtype = hold.charAt(0); // Hold descriptions consist of a hold type (S, P, E) ...
      int holdnumber = hold.substring(1).toInt(); // ... and a hold number
      int lednumber = ledmapping[holdnumber];
      Serial.print(holdtype);
      Serial.print(holdnumber);
      Serial.print(" --> ");
      Serial.print(lednumber);
      if (holdtype == 'S') { // Start hold
        strip.SetPixelColor(lednumber, green);
        Serial.println(" (green)");
      }
      if (holdtype == 'R' || holdtype =='P') { // Right hold
        strip.SetPixelColor(lednumber, blue);
        Serial.println(" (blue)");
      }
      if (holdtype == 'L') { // Left hold
        strip.SetPixelColor(lednumber, violet);
        Serial.println(" (violet)");
      }
      if (holdtype == 'M') { // Match hold
        strip.SetPixelColor(lednumber, pink);
        Serial.println(" (pink)");
      }
      if (holdtype == 'F') { // Foot hold
        strip.SetPixelColor(lednumber, cyan);
        Serial.println(" (cyan)");
      }
      if (holdtype == 'E') { // End hold
        strip.SetPixelColor(lednumber, red);
        Serial.println(" (red)");
      }
      
      if (pos == -1) { // Last hold has been processed!
        strip.Show(); // Light up all hold (and additional) LEDs
        problemstring = ""; // Reset problem string
        useadditionalled = false; // Reset additional LED option
        problem_loaded_time = millis(); // Set problem loaded time
        state = 0; // Switch to state 0 (wait for new problem string or configuration)
        Serial.println("---------\n");
        break;
      }

      problemstring = problemstring.substring(pos+1, problemstring.length()); // Remove processed hold from string
    }
  }

  // When "auto turn off" enabled, clear the LEDs after x minutes of inactivity
  if (AUTO_TURN_OFF && problem_loaded_time && millis() - problem_loaded_time > AUTO_TURN_OFF_MINUTES * 60000) {
    Serial.println("---------\n");
    Serial.println("Turning off LEDs due to inactivity");

    strip.ClearTo(black); // Turn off all LEDs in LED string
    strip.Show(); // Light up all hold (and additional) LEDs
    problem_loaded_time = 0; // Reset problem loaded time

    Serial.println("---------\n");
  }
}
