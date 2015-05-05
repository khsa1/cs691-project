
SOURCES=main.tex refs.bib motivation.tex algorithm.tex implementation.tex\
	abstract.tex keywords.tex results.tex conclusions.tex related.tex


main.pdf: $(SOURCES)
	pdflatex main
	bibtex main
	pdflatex main
	pdflatex main
