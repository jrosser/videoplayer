#!/bin/bash

abs_top_srcdir=`dirname $0`

# if version.c.in doesn't exist, extract the version from the distributed version.c
# otherwise, extract from git
if test -f ${abs_top_srcdir}/src/version.c.in
then
	GIT="git --git-dir=${abs_top_srcdir}/.git --work-tree=${abs_top_srcdir}/"
	VERSION_A=`${GIT} describe --always --long --tags HEAD`
	VERSION_B=`${GIT} describe --all --dirty=+dirty`
	VERSION_STRING="${VERSION_A}-${VERSION_B}"
else
	#const char* BUILD_VERSION = "v1pre-13-g9334e67-heads/v1+dirty";
	VERSION_STRING=`awk '/BUILD_VERSION/{print substr($5,2,length($5)-3)}' ${abs_top_srcdir}/src/version.c`
fi

if test "x$1" = "xraw"
then
	echo ${VERSION_STRING}
else
	# filtered version suitable for filenames
	echo ${VERSION_STRING} | tr '/:' '~$'
fi
