# NumberJack
Simple, lightweight live debugging for Arduino/ESP8266!

![#NumberJack Library](https://i.imgur.com/f0o1glZ.png)

## *Another* Serial Debugger?

Yes, but no. **NumberJack does things a little differently.** Most Serial debugging libraries print *all* information available so that an application can filter through it, whereas NumberJack sends **absolutely nothing** until the Windows app tells it which variables it should do live tracking on. This way your debugging tools can be left in production products and there's just a microscopic tax on the CPU since it won't spend any time printing NumberJack output when it's not connected.

### And when it *does* print? Will that slow my controller down to keep sending information?

NumberJack uses a special reporting syntax for when it needs to send variables. Let's say we have three we want to track:

- The **seconds** since boot, a measure of *TIME*
- The analog value of A0, where a **pressure** *SENSOR* is connected
- The boolean value of Pin 13, where a **motion** detector is connected; another *SENSOR*

**Once connected to this software, here is the handshake between (W)indows and (M)icrocontroller:**

W: **$NP?** *(is Numberjack Present on this device?)*

C: **$NP!** *(NumberJack is running!)*

W: **$NRM** *(Request map of variables)*

C: **$NM|TIME=0,SENSOR=1|seconds=0,pressure=1,motion=2|001** *(Variable map response)*

C: **$NLV|10100** *(NumberJack Library Version: 1.1.0)*

That "**$NM**" line is an important feature of NumberJack. You can combine Variables into Categories while writing your software. This way, if you have more than one type of device with NumberJack on it they can each get customized labels and sections within the GUI. These "Pretty Names" are sent upfront, and from then on they are referred to by IDs like 0,1,2,3,etc....

This keeps Serial traffic to the bare minimum!

### Seems like a lot of work...

Nope! Just one start line in **setup()**, one update in **loop()**, and one more line in **setup()** for every variable you want to track! Here's a version of the Arduino Blink.ino sketch with NumberJack added:

    #include <NumberJack.h>
    NumberJack jack;
    
    bool blink_state = false; // Our variable to track
    
    void setup() {
      jack.begin(230400,1);
      pinMode(LED_BUILTIN, OUTPUT);
      jack.track(&blink_state, t_bool, "Blink State", "Output", TRIGGERED);
    }

    void loop() {
      if(millis() % 1000 == 0){
        blink_state = !blink_state;
        digitalWrite(LED_BUILTIN, blink_state);
        delay(1);
      }
      jack.update();
    }

The "jack.**track**()" function is where the magic happens. Let's break it down:

    jack.track(
      &blink_state,  // The variable to track (A pointer/reference (&) to it's location in memory, actually)
      t_bool,        // The data type - in this case a boolean
      "Blink State", // The "Pretty Name" of this variable to be seen in the app
      "Output",      // The "Pretty Name" of the category this variable belongs to
      TRIGGERED      // A built-in alias for true, means this variable will be auto-reported by default
    );
    
Let's take a look at the Windows side again to see what that gets us:

![#NumberJack Application](https://i.imgur.com/O45i6w2.png)

On the left, we now see our variable "Blink State" and its live value inside our new category "Output". On the right is that "minimal" Serial output we discussed. Because "Triggering" was enabled on the blink_state variable, whenever its value has changed an update is sent to the debugger. (And never sent when it isn't - big savings on CPU time!)

These updates are shown in the Serial Monitor as:

    $NV|0|0|0
    $NV|0|0|1
    $NV|0|0|0
    $NV|0|0|1
    etc...

This is a highly simplified way of your Arduino saying:

    $NumberJack Variable | with ID 0 | in Category ID 0 | has changed to 0
    $NumberJack Variable | with ID 0 | in Category ID 0 | has changed to 1
    $NumberJack Variable | with ID 0 | in Category ID 0 | has changed to 0
    $NumberJack Variable | with ID 0 | in Category ID 0 | has changed to 1
    etc...

