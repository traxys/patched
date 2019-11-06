BIN = bin
OBJ = obj
SRC = src
DOC = doc
LATEXMK = pdflatex -output-directory $(OBJ)
RAPPORT = rapport
TARGET_FILES = rapport-modele.pdf # À REMPLACER PAR rapport.pdf
FILES = $(addprefix $(RAPPORT)/,$(TARGET_FILES))
CXX = clang++
CXXFLAGS = --std=c++17 -O3

all: bin doc rapport

bin: $(BIN)/computePatchOpt

rapport: $(FILES)
	
%.aux: %.tex
	$(LATEXMK) $<

%.pdf: dirs
	$(LATEXMK) $(@:.pdf=.tex)
	cp -f $(OBJ)/$(basename $(@F)).pdf $@

dirs:
	mkdir -p $(BIN)
	mkdir -p $(OBJ)

$(OBJ)/%.o: $(SRC)/%.cpp dirs
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BIN)/%: $(OBJ)/%.o dirs
	$(CXX) $(CXXFLAGS) -o $@ $<

doc:
	doxygen Doxyfile

clean:
	rm -rf $(OBJ)
	rm -rf $(BIN)
	rm -rf $(DOC)
	rm -rf $(FILES)

.Phony: clean all dirs doc rapport
