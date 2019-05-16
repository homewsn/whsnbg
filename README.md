### Wireless home sensor network broker-gateway (whsnbg)

This repository is a part of the [HomeWSN](http://homewsn.github.io) project.

Whsnbg is a C implementation of the MQTT server(broker), the MQTT-SN gateway and simple MQTT rules engine designed for embedded platforms with small memory requirements (like OpenWrt).


#### Supported features

MQTT server (broker):
* MQTT Protocol Specification Versions 3.1.0 and 3.1.1.
* Both plain TCP and Websockets connections on the selectable IPv4 network interface.
* SSL(TLS) encryption provided by the external library axTLS or OpenSSL.
* QoS level 0, 1 and 2.
* User authorization.
* Remote MySQL database support for the specific topics.

MQTT-SN gateway:
* MQTT-SN Protocol Specification Version 1.2.
* UDP connection on the selectable IPv4 network interface.
* QoS level 0, 1 and 2.
* `active`, `asleep` and `awake` states of the sensors.
* Remote MySQL database support for the specific topics.

MQTT rules engine:
* Rules are stored in the specific rules topic (`$SYS/rulesengine/rules`) and the external whsnbg.rules file.
* Rules can be modified on the fly by publishing to this specific rules topic.
* The rule set includes triggers (based on the cron or mqtt publish events), actions, conditions and the internal variables.

#### Building (Linux)

Download [the latest release](https://github.com/homewsn/whsnbg/releases) in tarball from github and untar it:
```sh
$ curl -L https://github.com/homewsn/whsnbg/archive/v1.4.tar.gz | tar zx
$ cd whsnbg-1.4
```
Or you can also clone whsnbg repository:
```sh
$ git clone https://github.com/homewsn/whsnbg.git
$ cd whsnbg
```
Then build possibly with [options](#build-options-linux) and install:
```sh
$ make
$ sudo make install
```
Configure the [execution options](#configuration-options):
```sh
$ sudo nano /usr/local/etc/whsnbg.conf
```
Change mqtt_iface from eth0 to the required network interface, for example enp0s3.
Change mqttsn_iface from eth0 to the required network interface, for example enp0s3 or sl0 etc.
You can always find out the network interfaces by running  `$ ifconfig`.

#### Building (Windows)

Download [the latest release](https://github.com/homewsn/whsnbg/releases) in zip from github and unzip it. Open the MSVS 2010 solution, build and run whsnbg.exe file.

#### Building (OpenWrt)

* Install [OpenWrt buildroot](http://wiki.openwrt.org/doc/howto/buildroot.exigence).
* Add the following line to the `feeds.conf.default` in the OpenWrt buildroot:
```
src-git homewsn https://github.com/homewsn/homewsn.openwrt.packages.git
```
* This feed should be included and enabled by default in the OpenWrt buildroot. To install all its package definitions, run:
```sh
$ ./scripts/feeds update homewsn
$ ./scripts/feeds install -a -p homewsn
```
* The packages should now appear in menuconfig. You can find and select whsnbg in the Network menu.
* Exit and save new configuration, then compile the package:
```sh
$ make package/whsnbg/compile V=99
```


#### Build options (Linux)

Build options can be set on the command line with the make command, for example:
```sh
$ make DAEMON_VERSION=1 DETAILED_LOG=1
```

| Option | Description |
| --- | --- |
| DAEMON_VERSION=1 | Builds the daemon version |
| DETAILED_LOG=1 | Enables detailed debug log |
| RULES_ENGINE=1 | Enables rules engine thread |
| WITH_DEBUG=1 | Adds GDB debug support |
| TLS_LIB_AXTLS=1 | Enables TLS support by axTLS library |
| TLS_LIB_OPENSSL=1 | Enables TLS support by OpenSSL library |
| SENSOR_DATA_DECODING=1 | Enables automatic decoding of the MQTT-SN message payload from the sensors |
| SENSOR_DATA_STORING=1 | Enables storing decoded data from the sensors in the MySQL database |
| MQTT_DATA_STORING=1 | Enables storing payload from the specific MQTT topics in the MySQL database |
| STATIC_LINK=1 | Builds the static link version |

#### Build options (Windows)

Build options should be set manually in the config.h file, for example:
```c
// Windows
// ** Manually configure ** >>>>>

#define USE_TLS_LIBRARY			// use external TLS library
// TLS library
#define OPENSSL_LIBRARY		// OpenSSL
//#define AXTLS_LIBRARY			// axTLS
//#define SSL_LIBRARY_HEADERS	// TLS library headers

...

// <<<<< ** Manually configure **
```

#### Build options (OpenWrt)

```sh
$ make menuconfig
```
Then find  Network -> whsnbg and press Enter. Configure options, setting to <*> or < >.

#### Configuration options
The execution configuration is in res/whsnbg.conf file (Winodws) or /usr/local/etc/whsnbg.conf (Linux) by default.
For Linux please make sure that you specify the correct mqtt_iface and mqttsn_iface parameters (they do not matter for Windows only). If incorrect interfaces are specified, then the program will end immediately.

```conf
# name = value
# maximum name length is 32 bytes
# maximum value length is 64 bytes

# MQTT section (tcp ports)
mqtt_iface = eth0
mqtt_port = 1883
mqtt_tls_port = 8883
mqtt_ws_port = 8082
mqtt_ws_tls_port = 8081

# MQTT-SN section (udp port)
mqttsn_iface = eth0
mqttsn_port = 1883

# MySQL section (remote MySQL database)
mysql_enable = 0
mysql_server = 192.168.0.213
mysql_user = whsnbg
mysql_password = some_pass
mysql_database = homewsn
mysql_port = 3306

# MQTT users section
mqtt_auth_enable = 0
user_name = name1
user_password = password1
user_publish_enable = 1
user_name = name2
user_password = password2
user_publish_enable = 0
```

#### Dependencies

* If you enable TSL support please be sure you have installed the appropriate library ([axTLS](http://axtls.sourceforge.net/) or [OpenSSL](https://www.openssl.org/)).
* If you enable storing payload in the MySQL database you will need [MySQL client library](http://dev.mysql.com/downloads/connector/c/).

#### License

[GNU GPL v 2.0](http://www.gnu.org/licenses/gpl-2.0.html)
