CXXFLAGS += -std=c++17
LDLIBS = -lgpiod

all:		neo-console

neo-console:	neo-console.cc nanohat.cc nanohat.h font.cc

font.cc:	bdfparser spleen-8x16.bdf
	./bdfparser > font.cc

bdfparser:	bdfparser.cc

install:
	strip neo-console
	cp neo-console /usr/local/bin
	cp neo-console.service /etc/systemd/system
	systemctl daemon-reload

clean:
	rm bdfparser
	rm font.cc
	rm neo-console

