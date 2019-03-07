

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
6. run `pio run --upload-port PORT_TO_UPLOAD`
    
    under linux, PORT_TO_UPLOAD is usually /dev/ttyUSB0.