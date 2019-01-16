#!/bin/bash
#
# make-client-certs.sh - creates the client certs for registering with Mumble
#
# Usage:
#
#   make-client-certs.sh <username>
#
#   make-client-certs.sh <userprefix> <count>
#
# Notes:
#
# * The certs are self-signed and are not passphrase protected. Depending on
#   the target environment and usage, this may or may not be OK. If you need
#   a passphrase, you'll need to hack Mumlib.
#
# * The names are hard-coded in mumsi to match <username>-key.pem and 
#   <username>-cert.pem. This is done to make it easier to configure multi-line
#   functionality.
#
# * When generating files for a series of users, the counter is appended to the
#   user name, from '0' to one less than the COUNT.

function usage {
    cat <<EOF
Usage:

    $0 username
    $0 user-prefix count
EOF
    exit 1
}

USER="$1"
COUNT="$2"

# In this 'format', the %s is replaced with the user name generated in
# the for loop.
SUBJFMT="/C=DE/ST=HE/L=Ffm/O=Mumble Ext./CN=%s"

if [ -z "$USER" ]; then
    usage
fi

if [ -n "$3" ]; then
    usage
fi

if [ -z "$COUNT" ]; then
    COUNT=1
fi

for ((i=0; i<$COUNT; i++)) {
    prefix="${USER}${i}"
    subj=$(printf "$SUBJFMT" $prefix)

    openssl req \
        -nodes \
        -new \
        -x509 \
        -keyout ${prefix}-key.pem \
        -out ${prefix}-cert.pem \
        -subj "$subj"
}


