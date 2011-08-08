#############################################################################
# Steve Rider - stever@area23.org
# Grufti - an interactive, service-oriented bot for IRC
#
# Makefile.  from the beginning.
#
#############################################################################

Grufti10: 
	@cd src; make

clean:
	@cd src; make clean

dist:
	@cd src; make dist

cgi:
	@cd src-cgi; make

install: Grufti10
	@sh ./install

install-cgi: cgi
	@sh ./install-cgi

xinstall: Grufti10
	@sh ./install 1

config:
	@cd configure; sh ./configure
