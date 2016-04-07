# *mumsi* - SIP to Mumble gateway

SIP to Mumble gateway based on PJSIP stack and *mumlib* library. It registers to SIP registrar and listens for incoming
connections on the SIP account.

This enables the user to participate in Mumble conference using SIP client or perhaps ordinary telephone, by VoIP provider.

## Dependencies

* Boost libraries
* *log4cpp*
* *pjsua2* from Pjproject SIP stack
* CMake
* *mumlib* - https://github.com/slomkowski/mumlib

## Build and usage

* Install all needed dependencies

* Clone and compile Mumlib library. Since it doesn't have any installer, clone it to common directory:
```
mkdir mumsi-dist && cd mumsi-dist
git clone https://github.com/slomkowski/mumlib.git
mkdir mumlib/build && cd mumlib/build
cmake ..
make
cd -
```

* Then clone and build *mumsi*:
```
git clone https://github.com/slomkowski/mumsi.git
mkdir mumsi/build && cd mumsi/build
cmake ..
make
```

* Copy example *config.ini* file and edit it according to your needs:
```
cp config.ini.example config.ini
```

Remember to add URIs which you want to make calls from. Calls from other URIs won't be answered.

* To run the service, type:
```
./mumsi config.ini
```

## Start at boot

*mumsi* provides no *init.d* scripts, but you can use great daemon mangaer, [Supervisor](http://supervisord.org/).
The sample configuration file:

```ini
[program:mumsi]
command=/home/mumsi/mumsi-dist/mumsi/build/mumsi config.ini
directory=/home/mumsi/mumsi-dist/mumsi
user=mumsi

stdout_logfile=/home/mumsi/console.log
stdout_logfile_maxbytes=1MB
stdout_logfile_backups=4
stdout_capture_maxbytes=1MB
redirect_stderr=true
```


## Issues

#### Port and NAT

Remember to allow incoming connections on port 5060 UDP in your firewall. If you're connecting to public SIP provider from machine behind NAT, make sure your setup works using some generic SIP client. Since SIP is not NAT-friendly by design, PJSIP usually takes care of connection negotiation and NAT traversal, but might fail. The most reliable solution is to configure port forwarding on your home router to your PC.

#### `PJ_EINVALIDOP` error

You may encounter following error when running *mumsi* on older distros

```
pjsua_conf_add_port(mediaPool, (pjmedia_port *)port, &id) error: Invalid operation (PJ_EINVALIDOP)
```

Some older versions of PJSIP are affected (confirmed for 2.3). In this case you have to update PJSIP to most recent version (2.4.5).


## TODO:

* multiple simultaneous connections
* outgoing connections
* text chat commands

## Credits

2015, 2016 Michał Słomkowski. The code is published under the terms of Apache License 2.0.

## Donations

If this project has helped you and you feel generous, you can donate some money to `14qNqXwqb6zsEKZ6vUhWVbuNLGdg8hnk8b`.
