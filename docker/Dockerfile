# UZDoom NX2 build environment (Nintendo Switch 2, kaiHEN).
#
# Base: the ounce-devkit image (devkitPro devkitA64 20260219 + libounce,
# OunceVK and libkaihen from the [ounce] pacman repository). Build it first
# from https://github.com/ouncebrew/ounce-devkit:
#   docker build -t ounce-devkit <ounce-devkit checkout>
#
# On top of it:
#   * host packages for the engine's native code generators (zipdir, re2c,
#     lemon), which run on the build machine;
#   * libsndfile 1.2.2 (devkitPro does not package it; ZMusic needs it), built
#     with the Switch 2 toolchain into portlibs/switch2.
#
#   docker build -f docker/Dockerfile -t uzdoom-nx2-build docker
FROM ounce-devkit

ARG LIBSNDFILE_TAG=1.2.2

ENV DEVKITPRO=/opt/devkitpro \
    DEVKITA64=/opt/devkitpro/devkitA64 \
    PATH=/opt/devkitpro/tools/bin:/opt/devkitpro/devkitA64/bin:/opt/devkitpro/portlibs/switch2/bin:/opt/devkitpro/portlibs/switch/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin

RUN apt-get update \
 && apt-get install -y --no-install-recommends git cmake ninja-build g++ python3 \
      libsdl2-dev libopenal-dev libvpx-dev libbz2-dev zlib1g-dev \
 && rm -rf /var/lib/apt/lists/*

# libsndfile with Ogg/Vorbis, FLAC and Opus. devkitPro's toolchain sets
# CMAKE_FIND_PACKAGE_PREFER_CONFIG and the portlibs only ship pkg-config
# files, so libsndfile's own find modules (fed by pkg-config) must be used.
RUN git clone -q --depth 1 --branch ${LIBSNDFILE_TAG} https://github.com/libsndfile/libsndfile.git /tmp/libsndfile \
 && sed -i "s/^if (CMAKE_FIND_PACKAGE_PREFER_CONFIG)/if (FALSE)/" /tmp/libsndfile/cmake/SndFileChecks.cmake \
 && . /opt/devkitpro/switch2vars.sh \
 && unset CFLAGS CXXFLAGS CPPFLAGS LDFLAGS \
 && export PKG_CONFIG_LIBDIR=/opt/devkitpro/portlibs/switch/lib/pkgconfig PKG_CONFIG_PATH= \
 && cmake -S /tmp/libsndfile -B /tmp/libsndfile/build-nx2 -G Ninja \
      -DCMAKE_TOOLCHAIN_FILE=/opt/devkitpro/libounce/cmake/Switch2.cmake \
      -DCMAKE_INSTALL_PREFIX=/opt/devkitpro/portlibs/switch2 -DBUILD_SHARED_LIBS=OFF \
      -DBUILD_PROGRAMS=OFF -DBUILD_EXAMPLES=OFF -DBUILD_TESTING=OFF -DENABLE_CPACK=OFF \
      -DENABLE_EXTERNAL_LIBS=ON -DENABLE_MPEG=OFF -DINSTALL_MANPAGES=OFF -DBUILD_REGTEST=OFF \
 && ninja -C /tmp/libsndfile/build-nx2 install \
 && grep -q "vorbis" /opt/devkitpro/portlibs/switch2/lib/pkgconfig/sndfile.pc \
 && rm -rf /tmp/libsndfile \
 && ( [ -f /opt/devkitpro/portlibs/switch2/lib/libdl.a ] || aarch64-none-elf-ar rcs /opt/devkitpro/portlibs/switch2/lib/libdl.a )

WORKDIR /src
