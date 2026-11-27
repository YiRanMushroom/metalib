# The compiler metalib is built with: GCC trunk at the commit of snapshot gcc-17-20261122 with every patch of this
# directory applied, installed in /opt/gcc-metalib, together with CMake, Ninja and git.
#
# Build the image, from the root of the repository (JOBS: parallel compile jobs, all cores when left out):
#   docker build -t metalib-gcc --build-arg JOBS=4 -f patches/Dockerfile patches
# Configure, build and test a checkout:
#   docker run --rm -v <checkout>:/src metalib-gcc sh -c \
#     "cmake -S . -B build -G Ninja && cmake --build build && ctest --test-dir build"

# What the compiler needs both while it is built and when it runs: assembler and linker, the C library headers, and
# the libraries cc1 and cc1plus are linked to. git fetches the GCC source in the build stage, and CMake's FetchContent
# calls it in the final image.
FROM ubuntu:24.04 AS base
RUN apt-get update \
 && apt-get install -y --no-install-recommends \
        binutils libc6-dev libgmp-dev libmpfr-dev libmpc-dev libisl-dev ca-certificates git \
 && rm -rf /var/lib/apt/lists/*

FROM base AS build
# The host compiler and the tools a build from git needs (flex: the release tarballs carry its output, git does not).
RUN apt-get update \
 && apt-get install -y --no-install-recommends g++ make flex gawk patch \
 && rm -rf /var/lib/apt/lists/*

# The master revision snapshot gcc-17-20261122 was made from (its LAST_UPDATED file). All 40 characters are needed:
# a server gives a single commit only by its full name.
ARG GCC_COMMIT=871ec62eab32d9d079279b95aa308c837b3a3c95
WORKDIR /gcc/src
RUN git init -q . \
 && git fetch -q --depth 1 https://github.com/gcc-mirror/gcc.git "${GCC_COMMIT}" \
 && git checkout -q FETCH_HEAD

COPY *.patch /gcc/patches/
RUN for p in /gcc/patches/*.patch; do echo "$p"; patch -p1 < "$p" || exit 1; done

WORKDIR /gcc/build
RUN ../src/configure --prefix=/opt/gcc-metalib --enable-languages=c,c++ --disable-bootstrap --disable-multilib \
        --enable-checking=release --disable-nls
ARG JOBS
RUN make -j"${JOBS:-$(nproc)}" \
 && make install-strip

FROM base
ARG CMAKE_VERSION=4.4.2
RUN apt-get update \
 && apt-get install -y --no-install-recommends curl ninja-build \
 && rm -rf /var/lib/apt/lists/* \
 && release="https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}" \
 && curl -fsSL -o /tmp/cmake.tar.gz "$release/cmake-${CMAKE_VERSION}-linux-$(uname -m).tar.gz" \
 && mkdir /opt/cmake \
 && tar -xzf /tmp/cmake.tar.gz -C /opt/cmake --strip-components=1 \
 && rm /tmp/cmake.tar.gz

COPY --from=build /opt/gcc-metalib /opt/gcc-metalib

# CMake takes the compiler from CC and CXX; the programs it builds find the compiler's libstdc++ through
# LD_LIBRARY_PATH.
ENV PATH=/opt/gcc-metalib/bin:/opt/cmake/bin:$PATH \
    CC=/opt/gcc-metalib/bin/gcc \
    CXX=/opt/gcc-metalib/bin/g++ \
    LD_LIBRARY_PATH=/opt/gcc-metalib/lib64

WORKDIR /src
CMD ["bash"]
