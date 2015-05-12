
SOURCES=main.tex refs.bib motivation.tex implementation.tex appendix.tex\
	abstract.tex keywords.tex results.tex conclusions.tex related.tex\
	acknowledgements.tex


main.pdf: $(SOURCES)
	pdflatex main
	bibtex main
	pdflatex main
	pdflatex main
