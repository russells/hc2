
IN = manual.tex dev-card.tex

PDF = $(IN:.tex=.pdf)

%.pdf: %.tex
	latexmk -pdf $<

%.png: %.xcf
	xcf2png -o $@ $<

XCFS = 	\
	HC-front-cover.xcf \
	SBLCDA2-blank.xcf \
	HC-demo.xcf \
	HC-demo-nobat.xcf \
	HC-display-parts.xcf \
	HC-display-parts-nobat.xcf \
	HC-buttons.xcf \
	HC-temp-cal-plus.xcf \
	HC-temp-cal-plus-nobat.xcf \
	button-enter.xcf \
	button-cancel.xcf \
	button-up.xcf \
	button-down.xcf \
	HC-max-MAX_T__.xcf \
	HC-max-SINCE__.xcf \
	HC-max-3_PM___.xcf \
	HC-max-5__HOTS.xcf \
	dev-hardware.xcf \
	arrow-down.xcf \
	arrow-left.xcf \
	arrow-right.xcf \
	arrow-up.xcf \
	ballmer.xcf

BUILT_PNGS = $(XCFS:.xcf=.png)

PNGS = $(BUILT_PNGS) dev-screenshot.png

TEX_OUTPUTS = \
	$(IN:.tex=.aux) \
	$(IN:.tex=.dvi) \
	$(IN:.tex=.log) \
	$(IN:.tex=.out) \
	$(IN:.tex=.toc)

LATEXMK_OUTPUTS = \
	$(IN:.tex=.fdb_latexmk)

.PHONY: default

default: $(PDF)

manual-book.ps: manual.pdf
	pdf2ps manual.pdf - | pstops '4:-3L(21cm,0)+0L(21cm,14.85cm),1L(21cm,0)+-2L(21cm,14.85cm)' /dev/stdin manual-book.ps

dev-card.pdf: dev-bitbucket.png dev-hardware.png dev-screenshot.png ballmer.png

manual.pdf: $(BUILT_PNGS)

.PHONY: clean
clean:
	rm -f $(PDF) $(BUILT_PNGS) $(TEX_OUTPUTS) $(LATEXMK_OUTPUTS)
