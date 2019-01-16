# Makefile
#
# This file generates the WAVE files from text files on macOS
#

WAVES := $(subst .msg,.wav,$(wildcard *.msg)) blow.wav

VOICE := --voice=Samantha
RATE := --rate=15
QUALITY := --quality=127


all: $(WAVES)

%.wav: %.aiff
	afconvert "$<" -d LEI16 "$@"

%.aiff: %.msg
	say $(VOICE) $(RATE) -o $@ < $<

#announce-new-caller.wav: /System/Library/Sounds/Blow.aiff
#	afconvert "$<" -d LEI16 "$@"


