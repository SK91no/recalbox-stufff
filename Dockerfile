FROM ubuntu:22.04
LABEL maintainer="digitalLumberjack <digitallumberjack@recalbox.com>"

ENV TERM xterm
ENV ARCH ''
ENV RECALBOX_VERSION 'development'
ENV RECALBOX_CCACHE_ENABLED ''
ENV PACKAGE ''

# Install dependencies
# needed ? xterm
RUN apt-get update -y && \
apt-get install -y tzdata && \
ln -fs /usr/share/zoneinfo/Europe/Paris /etc/localtime && \
dpkg-reconfigure --frontend noninteractive tzdata && \
apt-get -y install build-essential git libncurses5-dev qtbase5-dev qtchooser qt5-qmake qtbase5-dev-tools \
mercurial libdbus-glib-1-dev texinfo zip openssh-client libxml2-utils libpng-dev \
software-properties-common wget cpio bc locales rsync imagemagick bison flex bsdmainutils \
nano vim automake autopoint mtools dosfstools subversion openjdk-8-jdk libssl-dev libelf-dev \
graphviz python3 python3-matplotlib python3-numpy python3-six re2c libc6-dev-i386 libtool cabextract dos2unix \
p7zip-full xsltproc && \
rm -rf /var/lib/apt/lists/*

# Set the locale needed by toolchain
RUN echo "en_US.UTF-8 UTF-8" > /etc/locale.gen
RUN locale-gen

RUN mkdir -p /work
WORKDIR /work

CMD echo ">>> Setting recalbox version to ${RECALBOX_VERSION}" && echo "${RECALBOX_VERSION}" > board/recalbox/fsoverlay/recalbox/recalbox.version && \
    echo ">>> Reseting buildroot submodule" && ( cd buildroot && git reset HEAD --hard && git clean -dfx ) && \
    echo ">>> Fetching buildroot submodule" && ( git submodule update --init buildroot ) && \
    echo ">>> Making recalbox-${ARCH}_defconfig" && make recalbox-${ARCH}_defconfig && \
    export RECALBOX_CCACHE=${RECALBOX_CCACHE_ENABLED:+"BR2_CCACHE=y BR2_CCACHE_DIR=/share/ccache BR2_CCACHE_INITIAL_SETUP=--max-size=500G BR2_CCACHE_USE_BASEDIR=y"} && \
    RECALBOX_INSTRUMENTATION=${BR2_INSTRUMENTATION_SCRIPTS:+"BR2_INSTRUMENTATION_SCRIPTS=${BR2_INSTRUMENTATION_SCRIPTS}"} && \
    TOPLEVEL_JOBS="${TOPLEVEL_JOBS:-$(nproc)}" && \
    PKG_JLEVEL="${PKG_JLEVEL:-$(nproc)}" && \
    LOAD_LIMIT="${LOAD_LIMIT:-$(nproc)}" && \
    echo ">>> Make with command : make -j${TOPLEVEL_JOBS} -l ${LOAD_LIMIT} BR2_DL_DIR=/share/dl BR2_JLEVEL=${PKG_JLEVEL} $RECALBOX_CCACHE $RECALBOX_INSTRUMENTATION $PACKAGE" && \
    make -j${TOPLEVEL_JOBS} -l "${LOAD_LIMIT}" BR2_DL_DIR="/share/dl" BR2_JLEVEL="${PKG_JLEVEL}" $RECALBOX_CCACHE $RECALBOX_INSTRUMENTATION $PACKAGE
