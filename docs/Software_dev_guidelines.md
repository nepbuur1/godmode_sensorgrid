# Software development guidelines

## On libraries
- For starters, all subfolders of the libs folder can be ignored, except:
  - CleanCore, CleanRTOS, and if you need it, hx711 and LITTLEFS.
- Similarly, next folders from apps can be ignored:
  - _RunFromArduinoSketch, kattenvoermachine, keukenweegschaal, 
  kopie_van_wifi_scan_example_van_esp_idf, pong, pong_cleangui,
  TestHwLibGlcOled

### CleanRTOS
Note: the examples in CleanRTOS/examples show how CleanRTOS can be used.

### apps, main
- New apps can be added in subfolders of the apps folder (similar to existing apps there).
- The starting point of new apps can be added in main/main.cpp.
  Only the app that is to be built, is uncommented. the other apps are commented out (see main.cpp for examples). Note: in a similar way, main.cpp can be used to build/start an example of CleanRTOS.
- When you create a new app in the apps folder, create a subfolder in the apps folder with the name of the app. Add subfolders to it with the names src (for the sources of the app) and tests (for tests of parts of the app).
- In some cases, multiple devices communicate with each other. That may result in multiple apps. For instance, one app for the sender(s) and one app for the receiver(s).
In such case, those multiple apps work together in concert. To make that clear, create a parent folder in the apps folder for them, like "sensorgrid_v1", and put the app folders for the devices (like sensor_v1 and server_v1) within that parent folder.
- Of course MakeLists.txt should be kept up to date, such that all tests and applications can run at any timee, by uncommenting them in main.cpp.
- Those tests should be startable from main.cpp in a similar way as currently, the CleanRTOS examples.

### Arduino component
A dependency to the arduino component is included. So it is no problem to use arduino ide compatible libraries. However, don't use print and println. Instead, use the global logger that is declared in main.cpp for diagnostic or output logs.

## Your role
- You are a software development expert. 
- Make sure to chop up any design into clearly separated (preferably c++ class-based) objects. 
- Apply best code practices. 
- Periodically refactor to increase clarity of the structure. Provide comments where needed. 
- Use naming that is so clear that a lot of comments can be avoided.
- When something complex must be achieved, break it down in small, incremental, testable steps.
- Verify the quality / the success of each step by executing appropriate tests.
- Log summaries of your thoughts and what you've changed in the document "Log.md" in this folder (read its intro to get the idea).
- Within each app folder, add a doc subfolder, which itself has subfolders img (for svg images) and mermaid (for mermaid models), and a .md file with the name of the app itself. (for instance sensor_v1.md).
- The svg images should have a white (non-transparent) background.
- Put in that .md file a summary of what the app does, and add the object model to it (see Object_model_guidelines.md). The mermaid version of the object model is stored in the mermaid folder of the app. the .svg variation that is generated from that is stored in the img folder of the app. The .md file shows that .svg file as the object model, with below it, the object list.
- Furthermore, for each task, add a paragraph with the call-tree.
  (theoretical example):
  - ! update()
    - ! sendMessage()
      - ! init_wifi()
      - ? send(msg)
        - ! print(x)
    - ! log(time)

  So each function call is prepended with either ! or ?.
  ! is used when the function is always called when its parent function is called.
  ? is used when the functional optionally could be called when its parent function is called. 
- When a step is successfull, stage and commit it in the git repo with appropriate comments.


