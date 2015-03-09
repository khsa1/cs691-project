
SOURCES=main.tex refs.bib motivation.tex background.tex algorithm.tex \
	outline.tex programmatics.tex


main.pdf: $(SOURCES)
	pdflatex main
	bibtex main
	pdflatex main
	pdflatex main
