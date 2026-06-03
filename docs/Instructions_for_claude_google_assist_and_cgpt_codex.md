# Instructions for claude, google-assist and cgpt-codex

## Attached devices
Three ESP32-S3 devices are attached. They can be programmed using:
- idf.py -p /dev/ttyACM0 flash monitor
- idf.py -p /dev/ttyACM1 flash monitor
- idf.py -p /dev/ttyACM2 flash monitor

The first one will be referred to as the "collector". The second one as "sensor_1".
The third one as "sensor_2".

## Development phases
Please, follow the development phases in this document.
Do that, while adhering the software development guidelines as specified in Software_dev_guidelines.md in this folder.

### Phase 1
In the folder _not_part_of_this_project_reference_for_inspiration,
you can find a previous project that I created for esp32 (not esp32-s3).

In this phase, I'd like this project to be converted to esp32-s3:
- in the apps folder an app called collector_phase1 a corresponding app called sensor_v1.
- de code of sensor_v1 is used to program both the sensor_1 and the sensor_2 device.
  (after changing a #define for unique id).
- the original "sensor_node" was reading adc input. for now, change that, such that a number is sent which is modulo 1024, but incremented by 10*sensor_index for every message.(something like this for sensor_1: n+=10; send(n%1024), and for sensor_2:
n+=20; send(n%1024)).
- after you have succesfully created the apps and programmed the attached devices with it, notify that everything is ready.
- provide the instruction of how I can login on the hotspot to see the webpage.
- provide the instruction of how I can view the monitor output of the collector, via which I should be able to verify via the logs that it's working properly.
- of course, I also should be able to see it by logging in on the hotspot.

#### Phase 1b (already completed)
I've made some updates to guidelines for coding and documenting. I'd like you to take into account those updates for our current sensor/collector project. 
- You'll see I request a "parent app" folder within the apps folder. Please call it "sensorgrid_v1". 
- Then put the current sensor_v1 folder within it, and also the
  collector_phase1 folder, but rename it server_v1 (from hereon, I'd rather talk about the server than the collector.
- Please update any files to synchronize that). When you're ready, please don't forget to test if everything still builds and properly executes. (everything mentioned above can still be seen as part of phase 1 - a slight refactoring, and adding docs, without altering functionality)

#### Phase 1c
- Notice that I have added a fourth esp32-s3 device, which can be programmed using:
`idf.py -p /dev/ttyACM3 flash monitor`.
- Create an app for it within sensorgrid_v1, called "client_v1". The client_v1 device, app, should do following:
- Log in on the wifi hotspot of server_v1. Test the webpage(s) that it serves (with all functionalities), in a similar way as a human would test it via a browser.
- in its docs, so in sensorgrid_v1/client_v1/docs, add a test.md document that summarizes the tests that were done and their results.
- within the apps of sensorgrid_v1, in the docs, there are nice refers to .svg files in img folder. but the .svg files in question (which should be converted versions of the corresponding files in the mermaid folder) are missing. So I think a mermaid to svg tool should be downloaded, installed and used to fix that. Please also summarize how you did that in Log.md.

#### Phase 1d
Perhaps you can add a docs folder (with subfolders img and mermaid again) to sensorgrid_v1 as well, and add sensorgrid_v1.doc that summarises the complete sensorgrid_v1 - the role of it's apps, how they interact. This time including an "object model" where each of the apps is represented as an object, and along the arrows, the communications between them. (this is not a real object model - the stereo types within each object can be omitted, in this case).

### Phase 2
In phase 1, we created the project sensorgrid_v1, which consists of the cooperating apps client_v1, sensor_v1 and server_v1.
In this phase we start a new project, named sensorgrid_v2, which will have a folder structure that is similar to that of v1, but with client_v2, sensor_v2 and server_v2, in this case. sensorgrid_v1 can be looked at for ideas, but sensorgrid_v2 will be different in the way that server and sensors setup their communications and communicate. in sensorgrid_v1, sensors tried to send their message whenever they wanted. that'll probably go wrong when using a lot of sensors, and when the sensors send large datapackets (at the same moment or overlapping moment).
#### Phase 2a
Planning. Propose a planning of how sensorgrid_v2 could be made without the limitation stated above. Perhaps somehow, the server device could handshake with the client devices via espnow. It "says" to sensor1 metaphorically: go ahead, send what you've got. only then, sensor1 sends its data to the server. if the data is too much for a single packet, it may be distributed over multiple packets. when the server has received all datapackets from sensor1 (or it time-outs), it repeats the above for sensor2, etc. In advance, the server knows how much sensors there are, so it does not need to wait for non-existing sensor ids.
Is this idea viable? Or do you have suggestions for improvement of this plan?
#### Phase 2b
Implement sensorgrid_v2 with the following polling-based ESP-NOW protocol.

##### Protocol overview
The server operates in two phases:

**DISCOVER/REGISTER phase (initial startup only):**
- The server knows the expected number of sensors in advance (configured constant).
- The server broadcasts a `DISCOVER` message periodically.
- Each sensor that receives it replies with a `REGISTER` message containing its sensor ID. The server records the sensor ID and the sender's MAC address (available for free in the ESP-NOW receive callback metadata).
- The server stays in this phase until all expected sensors have registered. There is no timeout — it waits indefinitely.
- Once all sensors are registered, the server transitions to the POLL/DATA phase.

**POLL/DATA phase (normal operation):**
- The server polls each registered sensor in round-robin order.
- For each sensor, the server sends a unicast `POLL` message to that sensor's MAC address.
- The sensor responds with one or more `DATA` packets. Each DATA packet contains: sensor ID, packet index, total packet count, and payload.
- When a sensor receives a POLL, it extracts the server's MAC from the receive callback and auto-adds it as a peer if not already present. This allows sensors to recover transparently from a power cycle without needing re-registration in the normal case.
- After receiving all DATA packets from a sensor, the server moves to the next one.

**Sensor unresponsive — recovery without blocking healthy sensors:**
- If a sensor does not respond to a POLL, the server retries the POLL up to 5 more times.
- If the sensor still does not respond after 5 retries, the server marks it as "unregistered" but stays in the POLL/DATA phase.
- The server continues polling all remaining healthy sensors. Data from working sensors keeps flowing.
- Between poll cycles (after polling all healthy sensors), the server broadcasts a single `DISCOVER` to attempt re-registration of the missing sensor.
- Once the missing sensor responds with `REGISTER`, it is included in the next poll cycle.

**Onboard RGB LED status indicator:**
- LED off: all expected sensors are registered and responding. A healthy system shows no visible LED activity.
- LED flashing (~1 Hz): one or more sensors are not yet registered or have become unresponsive. This applies both during initial startup and during normal operation.

##### Packet types

| Type | Direction | Contents |
|------|-----------|----------|
| `DISCOVER` | server -> broadcast | message type |
| `REGISTER` | sensor -> server | message type, sensor ID |
| `POLL` | server -> sensor (unicast) | message type, sensor ID |
| `DATA` | sensor -> server | message type, sensor ID, packet index, total packets, payload |

##### What carries over from v1
- WiFi AP+STA mode on server (for web dashboard + ESP-NOW)
- WiFi STA mode on sensors (for ESP-NOW)
- Web dashboard with real-time bar charts and `/api/sensors` JSON API
- Download button (CSV export)
- client_v2 test app adapted for v2 endpoints
- Simulated sensor values (same incrementing pattern as v1)
- `neopixelWrite(RGB_BUILTIN, 0, 0, 0)` to turn off LED at startup (LED is then used only for status flashing)

##### Folder structure
```
apps/sensorgrid_v2/
    sensorgrid_common/    (shared packet definitions)
    sensor_v2/src/        (sensor app)
    server_v2/src/        (server app)
    client_v2/src/        (test client app)
    doc/                  (system-level docs, object models, mermaid, SVGs)
```
Each app gets its own `doc/` folder with object model, call trees, and (for client_v2) test results.

### Phase 3
#### Phase 3a
Now we move on to a new project, called sensorgrid_v3.
For starters, by fully copying sensorgrid_v2. (and renaming the sub-apps to server_v3, sensor_v3 and client_v3).

#### Phase 3b
In the end, we will make sure that each sensor (microcontroller) makes measurements, caches them, then waits for the POLL request of the server, upon which the measurements are sent to the server. Right now, only one "measurement"/testvalue is sent which (i believe) represented by a two-byte unsigned integer.
In the future, that will become a fixed amount of measurements (say 50), say all of type uint16_t. I guess it'd be most (processing-) efficient to send that these measurements as a single series of bytes in the (json) message rather than as separate measurement values. Allright, let's simulate that the sensor needs to spend 0.02 seconds of processing time to "measure" (for now) 50 values of type uint16_t. Let's use the same by 10 incrementing value as currently as the first "measurement" that is sent to the server. For now, let's say the server stores/caches the latest measurements from each sensor. For now, the webpage is kept the same. The values in the bars reflect only the value of the first cached measurement.

### Phase 4
Now we move on to a new project, called sensorgrid_v4.
For starters, by fully copying sensorgrid_v3. (and renaming the sub-apps to server_v4, sensor_v4 and client_v4).
#### Phase 4a
We now like to upgrade the webpage on our server.
Right now, it is a single page with bars and a download button, let's call that the home page.
I'd like to see that upgraded:  on the top, a navigation bar should be added, which allows to navigate to additional webpages. For starters, please add an additional webpage that visualises measurements in an other way. Let's say that it visualises the measurements of sensor1 (so only the first sensor) via a grid of rectangles. The gray-value of the rectangle is proportional to a measured value. the grid of rectangles consists of maximum 5 rectangles per row. The amount of columns is determined by the amount of measured values. So if 50 measured values are received, it results in 10 rows of 5 rectangles each. From this additional webpage (like from all future webpages), it is possible to navigate to the other webpages via a/the similar navigation bar at the top. It may be assumed that the webpages will be viewed in landscape, on a laptop (so no need for specific mobile support, for now).
#### Phase 4b
Let's further refine the latest additional webpage, which I will call the "grid view" page. Let's change the rectangles into circles. Furthermore, let's no longer order them in a rectangular fashion, but as follows: the first row has only one (the first) circle/measurement. the second row has two circles-/measurements. The third row has 3 of them. At some point, the row with most circles is reached. From thereon, each subsequent row contains one less circle. Up front, it should be calculated how many rows in total are thus needed to allocate all measurements (right now it's 50 of them, but in the future it could be another figure). For example, if there's 9 measurements, the rows would contain 1, 2, 3, 2, 1, so in total 9 circles. If there's 8 measurements, the rows would containe 1, 2, 3, 2 circels. If there's 16 measurements, the rows would contain 1,2,3,4,3,2,1, so in total 16 circles. If there's 13 measurements, the rows would contain 1,2,3,4,3 circles (because if the wides row would be 3, only 9 measurements would fit in the grid in total).
#### Phase 4c
Below the circle-grid, please put a histogram that shows the current distribution of the values in the circle-grid.
#### Phase 4d
Below the histogram, please put a table with 3 columns, one header row and one content row. The headers are called "max", "average", "sqrt(var)". 
The content row contains the figures for the measures currently shown in the grid: the maximum, the average and the square of the variance.
#### Phase 4e
On the same page, I'd also like two buttons. Each of them can toggle an aspect of the way the circle-grid is colored:
- One button "Normalize" can toggle whether the gray-range is normalised over the range of current measurements or not.
- Another button "Colorize" can toggle whether the gray-range is used, or a color-range (from black to blue to green to yellow to red).
- If "color mode" is selected while normalised mode is selected, the full color range is mapped to the current measurements.
#### Phase 4f
- Summarized, we have a color grid web page, with navigation bar on top, a normalize and colorize button, and for sensor 1 a group of widgets that visualize its measurements: the colorgrid, the histogram and the statistics table.
- What I would like now, is that similar groups of widgets are added to the same page for 3 more sensors: sensor 2, sensor 3 and sensor 4:
one row with on the left side the group of widgets of sensor 1 and to its right the group of widgets for sensor 2.
below that, another row with two groups of widgets: to the left of sensor 3, to the right of sensor 4.
#### Phase 4g
I realize now, that I'd rather have all 4 groups of widgets on a single row, rather than spread over 2 rows (becausee I assume that the webpage is viewed in landscape mode). Furthermore, let's increase the amount of "test measurements" that each sensor sends on each POLL from 50 to 64.
#### Phase 4h
The rows of circles of the circlegrids should be closer to one-another, such that the centers of all circles of the grid have equal distance (in all 6 directions, left, right, topleft, topright, bottomleft, bottomright, like in a hex grid)
#### Phase 4i
The grid view page currently fetches measurement data for each sensor via a separate HTTP request (`/api/measurements/1` through `/api/measurements/4`). Since the ESP32's WebServer is single-threaded, these 4 sequential requests slow down the update rate. Replace this with a single endpoint `/api/allmeasurements` that returns all sensor measurement data in one JSON response. Update the grid view JS to use this single endpoint instead of 4 separate ones.
#### Phase 4j
Please update the freertos tick rate to 1000Hz.
Furthermore, lower the JS polling interval to 100ms. Instead of `setInterval`, use a `setTimeout`-based loop: measure how long `fetchAll()` took, then wait only the remaining milliseconds to reach 100ms total cycle time. If `fetchAll()` took longer than 100ms, don't add extra wait.

### Phase 5
Now we move on to a new project, called sensorgrid_v5.
For starters, by fully copying sensorgrid_v5. (and renaming the sub-apps to server_v5, sensor_v5 and client_v5).
Right now, 5 devices can be accessed/programmed via their USB busses:
  Bus 001 Device 006: ID 303a:1001 Espressif USB JTAG/serial debug unit
  Bus 001 Device 005: ID 303a:1001 Espressif USB JTAG/serial debug unit
  Bus 001 Device 004: ID 303a:1001 Espressif USB JTAG/serial debug unit
  Bus 001 Device 003: ID 303a:1001 Espressif USB JTAG/serial debug unit
  Bus 001 Device 002: ID 10c4:ea60 Silicon Labs CP210x UART Bridge
The first one should be programmed as server_v5.
The second one should be programmed as client_v5.
The other three should be programmed as sensor_v5.

#### Phase 5a
I'd like to change the arrangement of the circles in the server that display 
the sensor values. The spacings can remain the same. Only the amount of circles
of each row should be different: instead of letting it increase and for the 
lower rows decrease again, let's add rows of equal amount of circles: 8 circles
per row. Still, the horizontal and vertical spacing between circles should remain
the same. Therefore, the odd rows should start at an x value that is larger than
that of the even rows (such that the circles are compactly stacked, without touching).

#### Phase 5b
The "sensors" we created so far, have shown "stub behaviour". It is the behaviour they should 
have when no devices are connected to them. However, sensors may have an MCP23017_ML and more.
If the MCP23017_ML is detected, the sensor should not send stub information, but instead 
send the real measurements. For your reference, I have added a folder with ScoliosePCB2,
with which I successfully tested standalone measurements of such a sensor.
In the code, the sensor data is stored in an array of max 8 rows and max 16 cols.
I'd like to see that when it sends its data to the server, that rows and cols are swapped.
So if the sensor is to send 8 rows and 16 cols, it should be sent in such a way that
the grid shows up on the server as 8 cols and 16 rows.
Make sure that stub and measure mode/behaviour are nicely separated. for instance over
multiple files.

#### Phase 5c
The Grid view on the server shows one panel for each sensor.
I'd like to add a button to each panel, called "remove offset".
When it is pressed, for each circle, the corresponding measurement value of that 
moment is stored as "offset" value. From hence on, the values represented 
by the circles are the measurement values minus the offsets.
An additional change is: for now, I'd like to invert the sequence in which the sensor 
sends its values to the server. (I think it'll mirror the circles-grid on the website
in both x and y direction, which I need to match the geometry of the pressure-sensors
on the hardware)

### Phase 5d
Okay, now, to the grid view web page, let's add a button next to Normalize and Colorize button, 
named "Max 2500". That button makes sure that the visualisation is normalized for a max value
of 2500. When the "Max 2500" button is toggled on, the normalize button is toggled off,
and vice versa (because both are different ways of normalizing, so mutually exclusive).
In addition, for now, let the stub-functionality of the sensors send values that are 
2500 at maximum, rather than 2^16-1.

### Phase 5e
Now, I'd like to add another button in de GridView, called "Capture".
At the moment it is pressed (it should not stay selected), from the sensorgrid that currently 
hold the maximum individual measurement value, it stores that max measurement value as
 "maxCaptured". Furthermore, from the same sensorgrid, it stores the sum of all its measurement values.
Both the maxCaptured value and the "maxSumCaptured" values should be displayed within textfields to the right
of where the buttons of the GridView reside.

To the right of the buttons of the GridView, I'd like to add an input/text field with a multiplier, called
"calibrate grams". The default value of calibrateGrams should be 1000. 
People can change that value by typing a new one in that field.

The current "Normalize" button, i'd like to rename it "Norm Display" and the Colorize button "Color Display".
Furthermore, the Max2500 button should be renamed "MaxFixed Display". For its functionality, it should no longer use 
the hardcoded 2500 value, but a value called fixedMax, which is from a text/input field called "Fixed Max Display" that is below the input fields of "Calibrate Grams". The default value of "Fixed Max Display" is 2500.
So the logic is: the "Norm Display" and "Max2500 display" select the way that the displayed values 
in the circles are translated to a range of color values or gray-levels without changing the displayed
numerical values. Both buttons cannot be "selected" at the same time. 
"Color Display" button selectes wether its color-range or gray-levels.

Speaking of which, I'd like to add another button, next to the Capture button, called "Norm MaxCap".
MaxCap. While "Norm MaxCap" is toggled on, the value shown in each of the circles should be: 
calibrateGrams*(measured value for that circle - offset_of_that_circle)/maxCaptured. (note: until the "remove offset" button is pressed
for the corresponding sensorgrid, offet==0 for each circle).
Similarly, I'd like an additional button called "Norm SumCap". While "Norm SumCap" is toggled on, 
the value shown in each of the circles should be: 
calibrateGrams/maxSumCaptured*(measured value for that circle - offset_of_that_circle)/maxCaptured. (note: until the "remove offset" button is pressed for the corresponding sensorgrid, offet==0 for each circle).

So when "Norm MaxCap" or "Norm SumCap" is selected, the numerical values in the circles are calculated
differently. Subsequently, how these values are visualised, depends on the states of the Buttons "Norm Display, Color Display and "Fixed Max Display".

Make sure that in the code, there can be never a division by zero (or it may crash - either esp or javascript in the browser).

### Phase 5f
At the bottom of each sensor panel, below the histogram, currently max, av and sqrt(var) are shown.
To the left of max, I'd like to display the Sum (of all values in the circles of the sensor panel).

More about those figures (sum, max, av, sqrt(var)).
I'd like these figures to be updated as follows: `sum_displayed = sum_displayed*filterFactor + (1-filterFactor)*sum_new`.
Let's select that filterfactor equal to 0.9 by default. Please allow to adjus it using an editable textfield. Just put it below the other editable text fields of the Grid View.

### Phase 5g
Below the stats of each panel, please add a "running plot", similar to an arduino-ide plot. use it to plot the 3 largest circle-values. The largest one in read, the middle one in green, the lowest one in blue. the horizontal axis corresponds to the 20 latest timeslots that the sensorgrid measurements arrived. The rightmost timeslot is most recent. the vertical axis correspond to the corresponding circle-value.
The top of the vertical axis should correspond to the top value that is used for displaying. So for instance, if Fixed max display == 1300 is filled in, with "MaxFixed Display" button selected, then the top of the vertical axis should correspond to a value of 1300.

### Phase 5h
Add a per-panel toggle button (e.g. "3D Surface") that switches the circle grid between the current 2D view and a 3D surface view. Use lightweight custom WebGL (no external libraries like Three.js) to keep the embedded HTML small enough for the ESP32's flash.

The 3D surface is a height-mapped mesh corresponding to the same grid of values displayed by the circles:
- The vertex positions form a grid matching the circle layout (e.g. 8 columns x 16 rows for 128 measurements).
- The height (Y-axis) of each vertex corresponds to the circle value at that position.
- The vertex colors use the same color/gray mapping as the 2D circle view (respecting Norm Display, MaxFixed Display, Color Display settings).
- All vertex normals point straight up, so the surface appears smooth under lighting.
- The vertical scale of the surface should correspond to the current display range (e.g. if MaxFixed Display = 1300, the top of the surface corresponds to 1300).

When toggled back to 2D, the circle grid is shown as before. The histogram, stats table, and running plot remain visible in both modes and continue updating regardless of which view is active.

So far, when we applied colors on the circles and 3d surface, the lowest value was represented by black. let's change that to medium-gray.

### Phase 5i
From now on, every sensor device may or may not have attached a hx711 with loadcell to it. If it has a hx711 with loadcell attached to it, then it is connected as follows: 
- **IO6** = voeding voor HX711
- **IO9** = ground voor HX711
- **IO7** = SCK
- **IO8** = DT / DOUT
For your reference, a working example has been included in the folder _not_part_of_this_project_reference_for_inspiration/Loadcell.
Now, on our gridview webpage, every sensor device has its own panel. at the bottom of each panel, there is statistical info and the running curves.
Below that, I'd like to add a section with UI for the loadcell, in case the sensor has a loadcell attached:
A field with the measured weight in grams. Below it, some widgets for calibration:
A tare button that can be pressed if there is no weight on the loadcell.
A "known weight" field, where a value in grams can be entered, indicating a known weight that is
currently on the loadcell. And a "Calibrate" button next to it, to indicate that the known weight
is currently on the loadcell. Using these fields, the calibration parameters that are calculated to 
show the correct weight in the measured weight field can be calculated. 

Implementation choicees for now: please store these parameters in a cookie, such that they can be 
initialialized the next time the webpage is visited and just transfer the raw measurement data from the hx711 
to the server/webpage.

### Phase 5j
On remove offset button press, after storing the offset values, I'd like to calculate the average of the grid-sum
over the following second. Let's call it "avResidualNoiseSum". That figure is to be displayed in a teext field below the maxSumCaptured textfield. I'd like to update next aspect from phase 5e:
While "Norm SumCap" is toggled on, the value shown in each of the circles should be: 
calibrateGrams/(maxSumCaptured-avResidualNoiseSum)*(measured value for that circle - offset_of_that_circle).
Of course, as always, make sure that division by zero cannot occur ever.
Furthermore, the values in the circles no longer need to be positive. So after subtracting offset levels, noise may cause temporarily negative values.

### Phase 5k
I'd like the browser to remember some settings of the Grid view page. I think the calibration parameters are already stored.
Only: the weight in the "known weight" field of the loadcell at the time the calibrate button of the loadcell was pressed for the last time, should be stored too (and still visible when returning to the webpage later). The same holds for : thee states of the top line of buttons in Grid View that have toggleable states: "Norm Display", "MaxFixed Display", "Color Display,", "Norm MaxCap" and "Norm SumCap", the fields maxCaptured, maxSumCaptured, avResidualNoiseSum, "Calibrate Grams", "Fixed Max Display" and "Stats Filter".

### Phase 5l
In a similar way that the stats are filtered, I'd like to filter each of the circle values, but with a separate filter constant.
Please, implement that, and allow thee new filter constant to be edited on a new textfield, called "Value Filter" (below the current "Stats Filter" field). Make its default value 0.5, and make sure that its latest setting is remembered when the webpage is visited lateron.

### Phase 5m
It is no longer needed to caputure max. The Norm MaxCap button can be removed from grid view. and the maxCaptured field can be removed from Grid View. The Capture button can be renamed to "Capture Sum"

### Phase 5n
I want next restructured: Instead of current functionality of Capture Sum button, maxSumCaptured Field and avResidualNoiseSum field, I'd like it updated as follows:
maxSumCaptured, avResidualNoiseSum and Calibrate Grams fields should be grouped together in a panel, at its current location. I call it "selected sensor info panel". as long as no sensor panel is selected (by clicking within a sensor panel), the "Selected sensor info panel" is hidden.
As soon as in any sensor panel is clicked, it is "selected". That is shown by changing the border color of the panel to green.
If another sensor panel is selected, the currently selected sensor panel is deselected (no longer border color green), and the contents of the "selected sensor info panel" shows the fields. The border of the "selected sensor info panel" is green, like the border of the selected sensor panel.

The "Capture Sum" button should also be hidden until a sensor panel is selected.

Calibrate grams can be entered by the user for the selected sensor panel
The "Capture Sum" button should capture maxSumCaptured (the sum of all shown circle values) and avResidualNoiseSum for the selected sensor only.

If "Norm SumCap" is not selected, the circles should represent the raw input values (as usual, using Value Filter). 
If "Norm SumCap" is selected, then the values that are displayed in the circles are calculated from the raw input values:
calibrateGrams/maxSumCaptured*(measured value for that circle - offset_of_that_circle).

calibrateGrams is the value in the "Calibrate Grams" input field (default 500).
maxSumCaptured is the value the maxSumCaptured text field, which is calculated by summing all offset-compensated circle values 
for the selected sensor panel at the time of clicking the "Capture Sum" button.

### Phase 5o
avResidualNoise should be updated only when the remove offset button is pressed, for the corresponding sensor panel.
not when the Capture Sum button is pressed

### Phase 5p
I'd like to rename the text "Calibrate Grams" in the "sensor info panel" to "Calibrate Sum Grams".
So far, so good, we then have a row with the "Capture Sum" button, the maxSumCaptured field, the avResidualNoiseSum field and the Calibrate Grams field, nicely with a green border. Which allows to provide the data for the "Norm SumCap" mode.

I'd like to prepare an alternative mode to calculate the circle values from the raw input values.
I'd like to toggle to that mode using the "Norm IndivCap", which can be selected by toggling a button with that name.
When "Norm IndivCap" is selected, "Norm SumCap" should be deselected. When "Norm SumCap" is selected, "Norm IndivCap" should be deselected.

Let me explain how the "Norm IndivCap" mode works:
For every panel, for every circle, 3 tuples ("capture value" , corresponding "calibrated indiv grams" value) can be stored.
How can we initialize these values:
If the user clicks on a circle, it gets a blue border (such that it is clear that it is selected).
Furthermore, below the "selected sensor info panel", a red-bordered "selected circle info panel" is then appearing.
It shows 3 rows, each with a "Capture Indiv" Button, and field-tuple, showing the current tuple values (capture value field above corresponding "calibrated indiv grams" value) and next to it is a "Calibrate Grams Indiv" field, where the user can fill in a value.
As soon as a "Capture Indiv" Button is pressed, the circle value of the currently selected circle is copied to the first member of the tuple behind it, and the value filled in in the corresponding "Calibrate Grams Indiv" field is copied to the second member of that tuple.

Lateron, I will explain how the "Norm IndivCap" mode will use these values to normalise the circle values from raw data in an alternative way, but for now, let's just implement the functionality described above, such that we can gather and store all data needed for it.

Wait.. I forgot a few additional updates: every circle for which all 3 "indiv captures" have been done, should have get a black border
(unless selected, then it temporarily becomes blue again).
Furthermore, the "sensor info panel" should get an additional button called "Reset Indiv Caps", which should reset the stored values of all (capturedValue,calibratedIndivGrams) tuples for all circles of the selected sensor panel to (0,0).

### Phase 5q
The data in both the "info selection panel" as wel as the tuples mentioned in phase 5p for each circle, should still be there if the webpage is opened a next time.

### Phase 5r
Right now, each Sensor panel has its own "remove offset" button. please, remove that button. Instead, the same should happen when a panel is selected, and from the "info selection paneel", a new to be added "Remove Offset" button is clicked.

### Phase 5s
Okay, now, what will happen in "Norm IndivCap" mode: The 3 tuples of each circle can be used to calculate the circle value
from the raw input value. If the 3 tuples are all uinitialized/zero, the circle value is the raw input value.
If one or more are not zero, they constitute a piece-wise linear lookup-figure, which runs from (0,0) (the origin) to subsequentially 
each of the tuples. After the last tuple should be linear extrapolation. In this figure, along the x-axis, the (offset compensated) raw input value (the value that is shown in the circle without Norm SumCap or Norm IndviCap selected) is filled in at the x-axis of the lookup-figure. The calculated value displayed in the circle thus becomes the corresponding (interpolated) value at the y-axis.
Note that this works if one or more tuples have been specified.

### Phase 5t
Great, now, if all tuples of a circle are zero, don't make its value the raw input value, but just make its value zero.

### Phase 5u
Below the current row of buttons (Norm display, MaxFixed Display, Color Display, Norm SumCap and NormIndivCap), I'd like another row of buttons and potentially other widgets (grouped with a grey border), which are dedicated to "record and play" functionality:
- A Record button. When pressed-in, it becomes red, stays in, and recording starts:
  All raw input data that is received from the sensors (both the gridsensor-info and the hx711 sensor-info) is efficiently
  stored in a large bytestream, frame after frame (I believe there's about 10 fps coming in).
- Next to the Record button, there is a Pause button. it can be toggled to pause recording.
- Next to the Pause button, there is a Stop button. it can be pressed to stop recording, and to "release/un-press-in" the Record button.
  (it is the only way to unpress the record button). The record button is no longer red, then.
- Next to the Stop button, there is a Play button. It can only be pressed after a recording was stopped. Up till then, it is grayed.
  If pressed, it becomes green. From then on, the view of the grid and loadcell of the sensor panels are being fed with the 
  stored raw input data from the past, rather than with the incoming sensor data via espnow. 
  So while playing, there is no need to poll for incoming sensor data.
- If new recording is started using the Record button, any previously recorded data is overwritten.

### Phase 5v
On the right side of the record panel discussed att phase 5u, I'd like to add a "Download" button.
It can be used to download the latest stopped or paused recording thus far, for now, lets's just download the json that you're currently storing the recorded data in.
As long as no recording has been made, or recording is in progress (not paused), that download button is grayed, and cannot be pressed.

### Phase 5w
Currently, when a sensor panel is selected, the "sensor info panel" with green border appears.
Next to the button "Reset Indiv Caps", I'd like to add a checkbox called "Enable Sensor".
By default, it is enabled. But if it is checked as disabled, with regard to data-polling, displaying in gridview
and recording, it will be treated as a sensor that is not powered/online.

### Phase 5x
Currently, only "loadcellRaw" is transmitted for each sensor, each frame.
I'd like to add to that "loadcellGram", which is the value in grams of the calibrated loadcell, as shown at that time as text value in the loadcell panel of the sensor.

### Phase 5y
Currently, every frame, circle values are calculated and EMA filtered using the "Value Filter" setting.
That is great, I certainly want to keep it that way.
But in addition, using the same filter setting, I'd like to calculate for every raw input value also it's ema-sibling, called ema-raw input, because I'd like to use it for "snapshots":
I'd like to add a "Snapshot" recording panel, below the Recording panel, with lila border:
- First button "Snapshot": like with record, but instead of storing the raw input values it stores the ema-raw values
  (which could either originate from inputs or prerecorded raw values that are being streamed, depending on whether
  recording playmode is active), and unlike with record, it adds them only exactly when the snapshot button got pressed (so not streaming).
- Second button "Clear Snapshots".
- Download button (allowing to download the snapshots, similar to downloading a recording).
- A textfield showing the amount of snapshots so far.
The row below it is meant for replaying snapshots:
- A button "Snapshot Replay"
When that button is pressed, it disappears and next buttons and widgets appear instead on its row:
- A button "Step Back"
- A button "Step Forward"
- A button "Goto First"
- A button "Goto Last"
- A button "Stop Snapshot Replay"
- A textfield that shows the currently selected Snapshot index, starting with 0.
In "Snapshot Replay mode", the (ema-raw) values (rounded to integers) of the snapshot are being used instead of the 
raw input values from recording or input polling, 10 times per second.
- The Back button lowers the index, unless it is already zero
- The Forward button increases the index, unless there are no snapshots left.
- Goto First resets the index to 0.
- Goto Last sets is to the last snapshot.
- "Snapshot replay mode" should be exited whenever "Stop Snapshot Replay" is selected, or when 
  "Clear Snapshots" is pressed.
- In that case, the source of raw input values for displaying depends on whether
  playing recording is still selected (uses recorded values) or not (uses polled input values).
- As long as there are no Snapshots, the "Snapshot Replay" button should be grayed / non-responsive.

### Phase 5z
Now, as for the sensor application, I'd like it to configure IO35 as input-interrupt with internal pullup resistor, which responds to 
a negative flank (caused by a button). Upon receiving the interrupt, the interrupt should be disabled until the next frame.
Moreover, the current frame, when sent to the server, should include the information "snapshotFrameRequest" equal to "true".
The server, as it polls its sensors, thus is notified via that information that a Snapshot is requested for the corresponding sensor.
The effect is that after having polled and processed the sensor values, prior to polling the next frame, it responds as if the Snapshot button was pressed for the sensor panel of the corresponding sensor (with the snapshotFrameRequest true). This thus offers a possibility "to press the Snapshot button" from a remote sensor.

### Phase 6
Now we move on to a new project, called sensorgrid_v6.
For starters, by fully copying sensorgrid_v5. (and renaming the sub-apps to server_v6, sensor_v6 and client_v6).
As no code changes, this won't require reprogramming of the devices.

### Phase 6a
Note: as always USB connected device(s) using cp210x have a pressure grid attached, and is/are only eligible to
program sensor_v6 on.
For this phase, only the server needs to be updated (you could check out which device is the server by listening to its serial output):
So far, after startup, the server waits for a fixed amount of sensors to join.
I'd like to change that (while retaining the option to revert to that behaviour via a #define):
From now on, I'd like the server to spend the first 10 seconds after booting to connect to all available sensors.
If after those 10 seconds, it has connected to at least one sensor, it can switch to operational mode. No need to wait for
additional sensors to join. If no sensor has joined yet, it indicates it as it already did when waiting for remaining fixed amount
of sensors by flashing the same red oled light. During the previously described first 10 seconds after boot, it
flashes the same light, with same frequency, but with green oled light. So in other words, when/as long no sensor is found,
after 10 seconds, the green flashing becomes red flashing.

### Phase 6b
Earlier, we implemented functionality to download recorded snapshots (using Download button in the Snapshots panel).
I'd like to the panel an Upload button, which allows to do the reverse upload a previously saved snapshot file,
such that the Snapshots functionality after that behaves, as if (only) the uploaded Snapshots were taken.

### Phase 6c
Please add similar functionality for the (stream-) Record panel.

### Phase 6d
To the snapshots panel, I'd like to use a horizontal slider. Moving the slider allows to quickly navigate to a snapshot. For instance if the slider is moved by the user to center position, the center snapshot is selected. The slider should only be visible when more than 3 snapshots are present.

### Phase 6e
I'd like to add an additional Pause button to the stream record panel, to the right of the Play button. When playing (Play button green), it can be Paused playing by pressing that Pause button. It can be unpaused by pressing that Pause Button once more.
Furthermore, I'd like a little additional space between the Stop (recording) button and the Play button to its right. I'd also like a little additional space to the left of the download button. that way, the grouping of the buttons becomes more clear:
the first group (Record, Pause, Stop) for recording. The second group (Play, Pause) for playing and the third group (Download, Upload) for file management.

### Phase 6f
To the record panel, I'd like to add a slider, similar as the one of the snapshot panel.
When playing is active (either paused or not), the slider should become visible. After moving the slider, the current frame being played should be set in accordance with the slider position. Conversely, while playing, the slider position should automatically update in accordance with the frame played.

### Phase 6g
Move the css part of the html file to a separate file, which is thus served separately.

### Phase 6h
Please move the 3d surface part of the html to a separate file, which is thus served separately.

### Phase 6i
Please alse move to separate files that are to be served separately:
- The Grid/circle rendering (hex grid creation, circle drawing, colorization)
- Record/Play functions (record, pause, stop, play, playpaused, upload, slider)
- Snapshot functions (take, clear, download, upload, replay, slider)
- Loadcell support (tare, scale, weight display, cookie persistence)
- Histogram drawing (bin counting, bar rendering)
- Plot drawing (running plot of top 3 values)

### Phase 6j
If Capture Sum button is pressed while Norm SumCap is enabled, don't let it do it's thing. Just show a error message: "Please first disable NormSumCap mode and wait a few seconds."
The error message disappears once Norm SumCap is disabled.
Similarly, if Capture Sum button is pressed while "Norm IndivCap" is enabled, similar behaviour.
Similarly, if Capture Indiv1, 2 or 3 is pressed while Norm SumCap or Norm IndivCap is enabled, apply similar behaviour.

### Phase 6k
Please add a bit additional spacing between next buttons:
- Color Display and Norm SumCap
- Stop and Play
- Pause and Download

### Phase 6l
From the statistics section (sum, max, average, sqrt(var)), please remove average and sqrt(var) and all calculations that were needed for it.

### Phase 6m
Currently, it is possible to select a single sensor-circle by clicking it (and the corresponding panel with red border then becomes visible). That's great. But in the future, it should be toggling rather than for all times. So when the sensor-circle is clicked once more, it becomes unselected again. (no longer blue border on the circle and the panel with red border hides again)

### Phase 6n
Currently, it is possible to select a single sensor-panel by clicking it (and the corresponding panel with green border then becomes visible). That's great. But in the future, it should be toggling rather than for all times. When **the background** of the sensor-panel is clicked (once more), it becomes unselected again. (the panel with green border hides again).

### Phase 6o
Now, I'd like to additional ways to toggle selection of a circle or sensor panel - by clicking on the background (so not on one of the widgets) of the corresponding red-bordered and green-bordered data entry-panel.

Once more, in more detail: 
0. if another sensor is selected (without selecting a circle), the current circle
should be deselected again and thus the accompanying red bordered panel should hide.
1. Only one panel can be selected at a time. 
2. Only one circle can be selected at a time. 
3. While a circle is in selected state, the corresponding sensor-panel should be in selected state too.
(not the other way round: a sensor panel can be in selected state while not (yet) having selected a circle. 
4. While a circle is in selected state <-> the corresponding red bordered circle-entry-panel should be visible. (and vice versa)
5. While a sensor panel is in slected state <-> the corresponding green bordered sensor-panel entry panel should be visible. (and vice versa).

### Phase 6p
Please reposition the panel with green border and the one with red border (when visible) below the collection of sensorgrid panels (currently, they are still positioned above them)

### Phase 6q
Now, when a recording has been made or uploaded (after pressing the Stop button in the record panel or after uploading a recording via the Upload button in the recording panel), then below the recording panel, a figure is shown, depending on whether a circle is selected:
1. when a circle is selected, the corresponding figure for that circle is shown (explanation on the figure can be found below).
2. when no circle is selectd, no figure is shown below the recording panel.
3. The figure has an x axis and an y-axis.
4. It shows all measurement data that is connected to the selected circle.
5. The x-axis is scaled in such a way, that all datapoints fit on the screen.
6. The y-axis shows the "circle value" of each datapoint for the circle, exactly as it would be calculated when the recording is played (and displayed on the circles of the sensorgrid, as currently implemented).
7. At the left of the y-axis, useful ticks (5 to 10) are shown. For instance, when max circle value for the selected circle is 105 grams, 7 ticks of 20 grams are shown: one at 0, one at 20, one at 40 .. one at 120.

### Phase 6r
Reconsidering, please only show the figure while next constraints are met simultaneously:
1. a circle is selected (like already implemented)
2. a recording has been made or uploaded (like already implememented).
3. the Play button is selected/green (and thus progress bar with slider are visible as well) (this is a new additional constraint)

### Phase 6s
Whenever the record-circle-figure (as implemented above) is visible, a white dot should be shown in the figure at the exact (x,y) location / datapoint of where the "play pointer / slider" currently is.

### Phase 6t
Great, now about that figure:
1. From now on, it should fit in the left 75% of the screen, rather than 100%.
2. The free 25% to the right of it should be filled with a similar figure:
  a. the y scale (and hor tick lines) are identical to that of the left figure.
  b. opposed to the left figure, it only shows a subrange of datapoints.
  c. the x range (the range of datapoints) is selected around the current position (as corresponding to the white dot in the left figure)
  d. by default, the x-axis is "zoomed in" by factor 10 with respect to the figure on the left.
  e. because of that, the right figure is effectively a "local zoom in" that only shows a zoomed in part of the left plot, for the subrange that fits, given the zoom-factor.
  f. The right figure also shows a white dot at the current data-point that is played.
  g. The x-coordinate of that white dot thus should always be at the center of the horizontal x-axis of the right figure.
  h. The right coordinate does not need to show the tick values of the y-axis next to it. 
     (because that is already shown next to the y-axis of the left figure)

### Phase 6u
Great, now, in the record bar, on the far right, I'd like to add a small zoom-slider, with following properties:
1. the zoom-slider can be used to change the zoom-factor. to the far left, the zoom-factor equals 10, to the far-right, the zoom-factor equals 1000. In between, log-interpolated values can be selected for the zoom-factor.
2. the zoom-slider is only visible when the "record-circle-value-figures" are visible.
3. the latest zoom-factor is stored, such that it is reapplied when the webpage is visited lateron.
4. to the right of the zoom-slider, the value of the resulting zoom-factor is displayed as a number (text).

### Phase 6v
Great, now, I'd like to add some small enhancements:
1. when the "record-circle-value-figures" are visible, the record panel should show the currently selected/played circle-value (as number, text) to the right of the text that displays the current frame number.
2. below the x-axis of the right "record-circle-value-zoomed-in-figure", I'd like to display a text that shows how 
many seconds the the displayed subrange represents. For example " <- 1.2s -> ". The subrange in seconds can be calculated 
from the subrange in frames and the amount of frames per second involved (if unknown, assume 10fps).
3. when the user holds the mouse button on a location of either of the record-circle-value figures, the 
current "play counter/position" is updated to the x-position/frame that corresponds to the location of the mouse in the figure.

### Phase 6w
On the main page, completely to the topRight (so to the right of the block: with Fixed Max Display, Stats
filter and Value Filter), I'd like to have two butons. The top one is [Upload Meta] the bottom one is [Download Meta]. When the download meta button is pressed, all metadata of this /grid page can be downloaded
as a json file. (so the state of whether Norm Display is enabled, the calibrations for each sensor, the 
calibration values for the loadcell, the Stats filter value, etc.). In a later point in time, the user
may use the "Upload Meta" button to upload these meta data, such that the webpage gets the same state
as before (apart from (recorded) sensor values or snapshots).

### Phase 6x
Great, now it would be great if when downloading and uploading recordings and snapshots, the same metadata
is included (at the end of the json file involved.)