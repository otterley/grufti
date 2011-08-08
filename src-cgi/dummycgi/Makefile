# Generated automatically from Makefile.in by configure.
#############################################################################
# Steve Rider - stever@transmission23.com
# Grufti - an interactive, service-oriented bot for IRC
#
# Makefile
#
#############################################################################

# CGI executable name (You may want to eventually change the path to your
# CGI directory so that you don't have to copy it over every time you compile)
CGI = dummy.cgi

CC = gcc
CFLAGS = -Wall
CLINKFLAGS = -s -lm 

# CGIC library stuff
LIBS = -L./ -lcgic
RANLIB = ranlib
AR = ar
XREQ = libcgic.a
DEBUG = -g
DEFINES =


# No need to change anything below this line
##############################################################################
OBJECTS = cgic.o main.o
SOURCES = cgic.c main.c

cgi: ${OBJECTS}
	${CC} ${CLINKFLAGS} -o ${CGI} ${OBJECTS}
	@chmod 755 ${CGI}
	@echo ""
	@echo "Successfile compile: ${CGI}"

.c.o: ; ${CC} ${CFLAGS} ${DEFINES} -c $<

libcgic.a: cgic.o cgic.h
	@rm -f libcgic.a
	${AR} rc libcgic.a cgic.o
	${RANLIB} libcgic.a

clean:
	@echo "Removing object files..."
	@rm -f core *.o *.a ${CGI}

depend:
	@echo "Updating dependencies..."
	@makedepend ${SOURCES}

# Our dependencies
# DO NOT DELETE THIS LINE -- make depend depends on it.
cgic.o: cgic.h
main.o: cgic.h
