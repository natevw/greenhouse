# RasPi compilation flags
CCFLAGS=-Wall -Ofast -mfpu=vfp -mfloat-abi=hard -march=armv6zk -mtune=arm1176jzf-s

# define all programs
PROGRAMS = greenhub
SOURCES = ${PROGRAMS:=.cpp}

all: ${PROGRAMS}

${PROGRAMS}: ${SOURCES}
	g++ ${CCFLAGS} -Wall -L../librf24/  -lrf24 $@.cpp -o $@

clean:
	rm -rf $(PROGRAMS)

.DEFAULT: all
