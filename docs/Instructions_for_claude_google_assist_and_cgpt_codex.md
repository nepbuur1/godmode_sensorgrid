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

### Phase 1:
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