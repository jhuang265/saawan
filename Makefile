CC=gcc

CXXFLAGS= -std=c99
ODIR = build
BINDIR = bin
SRCDIR = src
INCDIR = headers
CFLAGS=-I $(INCDIR) -lxml2 -lhpdf -lz -lm -g 
CFLAGSNOLINK = -I $(INCDIR) -lxml2 -lhpdf -g 

_DEPS = view.h error.h verify.h util.h layout.h pdf.h draw.h
DEPS = $(patsubst %,$(INCDIR)/%,$(_DEPS))

_OBJ = main.o view.o error.o layout.o util.o draw.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: $(SRCDIR)/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGSNOLINK) $(CXXFLAGS)

main: $(OBJ)
	$(CC) -o $(BINDIR)/saawan $^ $(CFLAGS) $(CXXFLAGS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(SRCDIR)/*~ 