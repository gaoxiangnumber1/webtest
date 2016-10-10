CC = gcc
INSTALLDIR = /usr/local/sbin
FLAGS = -std=c99 -g -Wall

all: webtest
.SUFFIXES: .c .o

SOURCE = webtest.c
OBJECT = $(SOURCE:.c=.o)
EXEC = webtest

.c.o:
	$(CC) $(FLAGS) -o $@ -c $<

$(EXEC): $(OBJECT)
	$(CC) $(FLAGS) -o $(EXEC) $(OBJECT)

clean:
	rm -f $(OBJECT) *~

install: $(EXEC)
	@if [ -d $(INSTALLDIR) ]; then \
		cp $(EXEC) $(INSTALLDIR);\
		chmod 555 $(INSTALLDIR)/$(EXEC);\
		echo "Installed in $(INSTALLDIR)";\
		echo "Thanks for your use webtest-Xiang Gao.";\
	else\
		echo "Sorry, $(INSTALLDIR) does not exist.";\
	fi

uninstall:
	@sudo rm $(INSTALLDIR)/$(EXEC);\
	echo "Uninstall webtest. Thanks for your use webtest-Xiang Gao.";
