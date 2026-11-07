#! /bin/bash

# Set build directory same as script location
export BuildDir="$(dirname "$0")"

# Set deployment target, architectures and SDK
export MACOSX_DEPLOYMENT_TARGET=10.6

export LDFLAGS="-arch i386 -arch x86_64 -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk"

export CFLAGS="-Os -arch i386 -arch x86_64 -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk"


# Build expat  =============================================
cd ${BuildDir}/expat
chmod +x configure
./configure --prefix=${BuildDir} --disable-dependency-tracking
./configure --enable-static
sudo make
sudo make install
sudo cp /usr/local/lib/libexpat.a ../Lib-Mac
sudo make uninstall
sudo make clean
# End ======================================================

# Build fontconfig  =============================================
cd ${BuildDir}/fontconfig
chmod +x configure
chmod +x install-sh
./configure --prefix=${BuildDir} --disable-dependency-tracking
./configure --enable-static
sudo make
sudo make install
sudo cp /usr/local/lib/libfontconfig.a ../Lib-Mac
sudo make uninstall
sudo make clean
# End ======================================================

# Build freetype  =============================================
cd ${BuildDir}/freetype
chmod +x configure
./configure --prefix=${BuildDir} --disable-dependency-tracking
./configure --enable-static  --with-bzip2=no
sudo make
sudo make install
sudo cp /usr/local/lib/libfreetype.a ../Lib-Mac
sudo make uninstall
sudo make clean
# End ======================================================

# Build libpng  =============================================
cd ${BuildDir}/libpng
chmod +x configure
chmod +x install-sh
./configure --prefix=${BuildDir} --disable-dependency-tracking
./configure --enable-static
sudo make
sudo make install
sudo cp /usr/local/lib/libpng16.a ../Lib-Mac
sudo make uninstall
sudo make clean
# End ======================================================

# Build pixman  =============================================
cd ${BuildDir}/pixman
chmod +x configure
chmod +x install-sh
./configure --prefix=${BuildDir} --disable-dependency-tracking
./configure --enable-static
sudo make
sudo make install
sudo cp /usr/local/lib/libpixman-1.a  ../Lib-Mac
sudo make uninstall
sudo make clean
# End ======================================================

# Build cairo  ============================================
cd ${BuildDir}/cairo
chmod +x configure
chmod +x build/install-sh
./configure --prefix=${BuildDir} --disable-xlib --disable-dependency-tracking
./configure --enable-static --enable-ft
sudo make
sudo make install
sudo cp /usr/local/lib/libcairo.a ../Lib-Mac
sudo make uninstall
sudo make clean
# End ======================================================


