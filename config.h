#ifndef __CONFIG_H
#define __CONFIG_H

#ifdef Q_OS_UNIX
#define DEFAULT_FONTFILE "/usr/share/fonts/truetype/ttf-dejavu/DejaVuSans-Bold.ttf"
#endif

#ifdef Q_OS_MACX
#define DEFAULT_FONTFILE "/Library/Fonts/GillSans.dfont"
#endif

#endif
