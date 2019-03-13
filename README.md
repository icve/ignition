
## Project status
The main features are completed, additional features are to be added.
    
### TODO:
* ntp client
* fix rgb driver
* cleanup, refactor, cleanup debug statements
* brghtness adjust support
* multiplex scanning support
* rgb tea light
* clock overflow animation
* wifi password
* tcp ssl authenti


### completed:
* clock display
* wifi tcp server
* communication driver (mcu to rtc, mcu to shift reg)
* script (tcp client) to set time



## compiling & uploading
The software can be compiled and uploaded to the MCU (esp8266) using pio(platformio) command-line tools / ide plugin (gui)

### with `pio` command-line tool
#### Setting up `pio`
1. git clone the project file (not required if you alread have the files)
2. (optional; recommended;)
create python virtual environment (with virtualenv, or python -m venv) and activates the virtual environment; you will have to reactivate the environment if you restart your terminal;
3. install `platformio` with pip

#### compiling
4. `cd` into ignition dir (one where `platformio.ini` is in, run `ls` to check)
5. run `pio run` to compile the software. (first run may take longer since `pio` also install the requires toolchain)

    issues you may run into:

        `command not found`: remember to activate your python environment

#### uploading
6. unplug mcu from board
7. connect mcu to computer
8. run `pio device list` to find the port mcu is connected to.
    
    if the device is not found, you may need to install the driver,
    install the CH340 driver and restart machine (driver can be found on the d1 mini website. [link to driver](https://wiki.wemos.cc/downloads))
9. run `pio run --target upload --upload-port PORT_TO_UPLOAD`
    
    * omit `--upload-port PORT_TO_UPLOAD` to let pio auto detect port.

    * add `--target monitor` to open terminal after upload 

    * under linux, PORT_TO_UPLOAD is usually /dev/ttyUSB0.