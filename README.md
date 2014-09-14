Whsnbg
=========
(Wireless home sensor network broker-gateway)

Whsnbg is a C implementation of the MQTT broker, the MQTT-SN gateway and simple MQTT rules engine designed for embedded platforms with small memory requirements (like OpenWrt).


Supported features
-----------

MQTT broker:
* MQTT Protocol Specification Version 3.1;
* both plain TCP and Websockets connections on the selectable IPv4 network interface;
* SSL(TLS) encryption provided by the external library axTLS or OpenSSL;
* QoS level 0, 1 and 2;
* user authorization;
* remote MySQL database support for the specific topics (if defined MQTT_DATA_MYSQL in config.h);

MQTT-SN gateway:
* MQTT-SN Protocol Specification Version 1.2;
* UDP connection on the selectable IPv4 network interface;
* QoS level 0, 1 and 2;
* "active", "asleep" and "awake" states of the sensors;
* remote MySQL database support for the specific topics (if defined SENSOR_DATA_MYSQL in config.h);

MQTT rules engine (if defined RULES_ENGINE in config.h):
* rules are stored in the specific rules topic ("$SYS/rulesengine/rules") and the external whsnbg.json file;
* rules can be modified on the fly by publishing to this specific rules topic;
* the rule set includes triggers(based on the cron or mqtt publish events), actions, conditions and the internal variables;


Building
-----------

* Edit config.h header file to choose the appropriate TLS library; to enable or disable automatic sensors data decoding, MySQL and rules engine support; to edit the pathes to the configuration(whsnbg.conf) and rules(whsnbg.json) files.
* Windows: open visual studio solution, build.
* Ubuntu: "make" or "make release" in the "whsnbg/src" directory.
* OpenWrt Buildroot: copy "whsnbg" directory to "openwrt/package", run "make menuconfig" from the "openwrt" directory, select "whsnbg" in the Network category, exit and save the config. Then build as usual "make package/whsnbg/compile V=s".
* Edit and copy whsnbg.conf and whsnbg.json(optional) files to the appropriated directory, run the program.


License
-----------

GNU GPL v 2.0
