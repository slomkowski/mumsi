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

## Configuring

### Multi-Line Support

If your SIP provider allows multiple simultaneous calls, mumsi can be configured to accept
calls and map them to separate Mumble users. The max\_calls is configure in *config.ini*:

```
[sip]
...
max_calls = 32
...
```

Currently, the Mumble connections are established at server start. For usability, the following
options are recommended:

* caller\_pin
* autodeaf
* channelAuthExpression

The maximum number of calls is set in *main.hpp* and should not exceed the
*PJSUA_MAX_CALLS* in *pjsua.h*, which by default is 32. This can also be recompiled to
more, if desired.

When mumsi logs into Mumble, it uses the user name from *config.ini* and appends 
the character '-', followed by the connection number (counter).

*LIMITATIONS:* The code is _alpha_ and needs testing/debugging, especialy in
the code that uses mumlib::Transport. Also, there is initial work on connecting
the Mumble user only when the SIP call is active, so the UI for other users is 
better, but this code is still very buggy and therefore disabled.

### Caller PIN

When the caller\_pin is set, the incoming SIP connection is mute/deaf until the
caller enters the correct PIN, followed by the '#' symbol. On three failed 
attempts, the SIP connection is hung up. On success, the Mumble user is moved
into the channel matching channelAuthExpression, if specified, and then mute/deaf
is turned off. As a courtesy to the other users, a brief announcement audio
file is played in the Mumble channel.

The caller\_pin is configured in *config.ini* in the *app* section:

```
[app]
caller_pin = 12345
```

In addition to the caller\_pin, a channelAuthExpression can be set. After
the caller authenticates with the PIN, the mumsi Mumble user will switch
to the Mumble channel that matches this expression. When the call is
completed, the mumsi Mumble user will return to the default channel that
matches channelNameExpression.

This helps keep the unused SIP connections from cluttering your channel.

### Autodeaf

By default (i.e. autodeaf=0), other Mumble users can only see whether the mumsi
connection has an active caller if they are in the same channel. This is becaue
the 'talking mouth' icon is not visible to users in other channels. The mute/deaf
icons, on the other hand, can be seen by Mumble users when they are in different
channels, making it easier to spot when a new caller has connected.

Setting `autodeaf=1' causes the mumsi Mumble user to be mute/deaf when there
is no active SIP call.

### Audio Files

When certain events occur, it is user-friendly to provide some sort of prompting
confirmation to the user. An example set of WAV files is provided, but they
can easily be customized or replaced with local versions, if needed. If the
files are not found, no sound is played. The following events are supported:

- welcome: Played to caller when first connecting to mumsi
- prompt\_pin: Prompt the caller to enter the PIN
- entering\_channel: Caller entered PIN and is now entering the Mumble channel
- announce\_new\_caller: Played to the Mumble channel when adding a new caller
- invalid\_pin: Let the caller know they entered the wrong PIN
- goodbye: Hanging up on the caller
- mute\_on: Self-mute has been turned on (not implemented)
- mute\_off: Self-mute has been turned off (not implemented)
- menu: Tell caller the menu options (not implemented)

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

#### mumlib::TrasportException

The multi-caller code is _alpha_ and needs testing/debugging, especialy in
the code that uses mumlib::Transport. Also, there is initial work on connecting
the Mumble user only when the SIP call is active, so the UI for other users is 
better, but this code is still very buggy and therefore disabled.

## TODO:

* multiple simultaneous connections
* outgoing connections
* text chat commands

## Credits

2015, 2016 Michał Słomkowski. The code is published under the terms of Apache License 2.0.

## Donations

If this project has helped you and you feel generous, you can donate some money to `14qNqXwqb6zsEKZ6vUhWVbuNLGdg8hnk8b`.
