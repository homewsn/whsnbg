#### Sensor simulator (sensim) and sensor simulator with GUI (sensimgui)

Sensor simulator (sensim) and sensor simulator with GUI (sensimgui) are designed fot testing MQTT (tcp) and MQTT-SN (udp) network command interfaces of whsnbg.

##### Sensim

Sensim is a console application for automated testing. Allows you to select a test suite that simulates the connection of the sensor or actuator by the mqtt-sn protocol, while commands from the "control panel" are sent via the mqtt protocol.

Building and testing (Linux):

Download [the latest release](https://github.com/homewsn/whsnbg/releases) in tarball from github and untar it. Then build sensim:
```sh
$ curl -L https://github.com/homewsn/whsnbg/archive/v1.3.tar.gz | tar zx
$ cd whsnbg-1.3/utils/sensim
$ make
```

Open first terminal window and run whsnbg:
```sh
$ whsnbg
```
Please note:
* whsnbg must be built with `SENSOR_DATA_DECODING=1` option.
* whsnbg must be restarted each time before running sensim.

Then open second terminal window and run sensim:
```sh
$ ./sensim -a 127.0.0.1 -p 1883 -s 1
```
where:
* `127.0.0.1` is an IP address of the network interface where whsnbg is ran.
* `1883` is a number of mqtt(tcp) and mqq-sn(udp) ports (must be the same).
* `1` is a test suite number (must be between 1 to 3). 1 - general connection test suite, 2 - sleep sensor test suite, 3 - active actuator test suite.

Building and testing (Windows):

Download [the latest release](https://github.com/homewsn/whsnbg/releases) in zip from github and unzip it. Open the MSVC++ 8.0 solution and build.

Open first command prompt window and run whsnbg.
```sh
> whsnbg
```
Please note:
* whsnbg must be built with `#define SENSOR_DATA` in the config.h file.
* whsnbg must be restarted each time before running sensim.
 
Then open next command prompt window and run sensim:
```sh
> sensim -a 127.0.0.1 -p 1883 -s 1
```
where:
* `127.0.0.1` is an IP address of the network interface where whsnbg is ran.
* `1883` is a number of mqtt(tcp) and mqq-sn(udp) ports (must be the same).
* `1` is a test suite number (must be between 1 to 3). 1 - general connection test suite, 2 - sleep sensor test suite, 3 - active actuator test suite.


##### Sensimgui

Sensimgui is a Windows desktop application for manual testing of whsnbg. The application allows to send and receive mqtt-sn messages to/from whsnbg individually.

Building and testing (Windows):

Download [the latest release](https://github.com/homewsn/whsnbg/releases) in zip from github and unzip it. Open the MSVC++ 8.0 solution and build sensimgui.

Open command prompt window and run whsnbg.
```sh
> whsnbg
```
Please note:
* whsnbg must be built with `#define SENSOR_DATA` in the config.h file.

Then run sensimgui and choose the command you want to send whsnbg.

Dependences:
* ATL
* WTL (source code is included)


##### License

[GNU GPL v 2.0](http://www.gnu.org/licenses/gpl-2.0.html)
