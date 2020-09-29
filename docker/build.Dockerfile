FROM ubuntu:20.10

# install tools
# note, g++ is required to build Boost.Build, xz-utils are required for extract from xz
RUN apt-get update
RUN apt-get install -y wget g++ cmake unzip git xz-utils libfindbin-libs-perl gcc-arm-none-eabi


# prepare the build root dir
ENV build_root=/build
WORKDIR /${build_root}

# getting the cross compiler
ARG cross_compiler_id=gcc-arm-8.3-2019.03-x86_64-arm-linux-gnueabihf

RUN wget -w5 https://developer.arm.com/-/media/Files/downloads/gnu-a/8.3-2019.03/binrel/${cross_compiler_id}.tar.xz
RUN tar xvf ${cross_compiler_id}.tar.xz
ENV PATH $PATH:/${build_root}/${cross_compiler_id}/bin

# cross compile boost
ARG boost_version=boost_1_74_0
ARG cross_compiler_executable=arm-linux-gnueabihf-g++

RUN wget -w5 https://dl.bintray.com/boostorg/release/1.74.0/source/${boost_version}.tar.bz2 && tar xvjf ${boost_version}.tar.bz2
WORKDIR /${build_root}/${boost_version}
RUN ./bootstrap.sh
RUN sed -i 's/^\s*using gcc.*$/    using gcc : arm : '${cross_compiler_executable}' ;/' project-config.jam
RUN ./b2 link=static runtime-link=static install toolset=gcc-arm --prefix=/opt/arm

WORKDIR /${build_root}
COPY build.sh ./

CMD ./build.sh
