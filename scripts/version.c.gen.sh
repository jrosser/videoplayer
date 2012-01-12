#!/bin/sh

scriptdir=`dirname $0`

sed -e 's!__VERSION_STRING__!'`$scriptdir/version-gen.sh raw`'!'
