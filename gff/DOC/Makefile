# Makefile for running a TeX document

# define suffix rules: get .dvi from .tex files; get .ps from .dvi files
.SUFFIXES: .tex .dvi .ps .pdf

STYLEFILES = skd.sty

TEXFILES = doc.tex

EPSFILES = 

# generate the Postscript file from the .dvi file
# (review .ps using ghostscript)

.dvi.ps: 
	dvips  $< -o $*.ps


.ps.pdf:
	ps2pdf $< $*.pdf

FILE = doc
PS = $(FILE).ps
DVI = $(FILE).dvi
PDF = $(FILE).pdf
EXPENDABLES = *.dvi *.log *.aux *.blg *BAK

$(FILE).ps  : $(FILE).dvi 

$(FILE).pdf : $(FILE).ps

ghost :
	ghostview $(PS) &

xdvi :
	xdvi $(DVI) &

pdf : $(PDF)

# use TeX to generate the .dvi file from .tex source files
# run latex twice to get cross-references inserted correctly
# (review .dvi using xdvi)

$(FILE).dvi: $(TEXFILES) $(EPSFILES) $(BIBTEXREFS) $(STYLEFILES)
	latex $(FILE)

# "make clean" will remove everything that can be regenerated
clean:
	/bin/rm -f $(EXPENDABLES)

realclean:
	/bin/rm -f $(EXPENDABLES) $(FILE).bbl $(FILE).toc

# "make all" will rebuild everything from scratch
all: clean
	make $(PS)
