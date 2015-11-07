# mumsi - SIP to Mumble gateway

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

* Then clone and build Mumsi:
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

* To run the service, type:
```
./mumsi config.ini
```

## Issues

* remember to allow incoming connections on port 5060 UDP in your firewall

* PJSIP packages in some distributions require to have a sound card in your system, which can be an issue on the server.
In this case, use *snd-dummy* sound module.

## TODO:

* SIP authentication - for now it answers every call
* multiple simultaneous connections
* outgoing connections
* text chat commands

## Credits

2015 Michał Słomkowski. The code is published under the terms of Apache License 2.0.
