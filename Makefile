CC = gcc
INSTALLDIR = /usr/local/sbin
CFLAGS =	-std=c99 -Wall -Wconversion -Werror -Wextra -Winline \
					-Wno-unused-parameter -Wpointer-arith -Wunused-function \
					-Wunused-value -Wunused-variable -Wwrite-strings

all: webtest

.SUFFIXES: .c .o

SOURCE = webtest.c
OBJECT = $(SOURCE:.c=.o)
EXEC = webtest

$(EXEC): $(OBJECT)
	$(CC) -o $@ $^

.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<

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
