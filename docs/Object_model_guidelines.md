# Object Model Guidelines

An **Object Model** is a diagram that shows how software objects—each with clearly defined, individual responsibilities—collaborate to ensure that requirements are fulfilled.

## Criteria

- As a rule of thumb, start with **one control object per use case**. This object is responsible for orchestrating the use case.
- Objects are **rectangles** containing only the **name** and the correct **stereotype**. Names are written in **class-name style (PascalCase)**.
- Relationships between objects are **arrows**, annotated with what they “say” to each other, written in **function-name style (camelCase)**.
- What they say to each other are:
  - **notifications** (e.g., `buttonPressed(buttonId)`),
  - **commands** (e.g., `activeerSolenoid`),
  - **questions** (e.g., `isButtonDown`).
  
  So **NOT** `button(buttonId)` or `solenoidActivatie()`.
- When requesting information, use a **return value** instead of a function parameter.  
  So not `getTemperature(temp)` but `getTemperature()`.
- Information passed along with a notification or command is included **in parentheses**, e.g. `buttonPressed(buttonId)`.
- The function attribute of passed information should reflect its **meaning**, not its **type**.  
  So not `buttonPressed(int)`, but `buttonPressed(buttonId)`.
- Make sure what is “said” to another object is appropriate for the object closest to (or acting as) the **boundary**.  
  For example: don’t tell object `RedLight` “`unsafeSituationDetected`”, but rather `enable` or `goOn`.  
  A button, on the other hand, tells a control object `buttonPressed(buttonId)` rather than `goLeftOrRight(buttonId)`.  
  This maximizes reusability.
- At minimum: **one control object per use case**.
- An object model always describes the software of **one device**. If multiple devices communicate, they do so via **proxy objects**, e.g. `CloudStorageProxy`, and for each device, there is a separate object model.
- Prefer not to use messages that contain only a single boolean parameter. It’s better to use two messages, e.g. `enable` and `disable`.
- Ensure naming in the object model aligns with the use case it is derived from.  
  Example: UC “configure” → `ConfigureControl`; UC “paying” → `PaymentControl`.
- Do not let controllers poll. Let boundary sensor objects do that themselves. Sensors usually **send** information (outgoing arrows from the sensor), instead of others explicitly requesting it.

# Object List

A list of the objects used in the Object Model. It should be put in the .md file below the image of the object model.

## Criteria

- In the description of each object, state only its **own responsibility**, independent of the rest of the system.
- So not: “`ButtonA` sends `buttonPressed` messages to `ControllerB`.”  
  But describe what it does by itself, without explicitly naming other objects in the system:  
  `ButtonA` represents the software side of a push button that automatically springs back when released.
