
all:	APL_demo.html index.html apl-intro.html translate Book.tc.log

APL_demo.html:	../../workspaces/APL_CGI.apl make_APL_demo
	./make_APL_demo

index.html:	../../workspaces/APL_CGI.apl
	$< > $@

apl-intro.html:	apl-intro make_intro translate
	./make_intro $<

translate:  translate.cc
	g++ -g2 -o $@ $<

Book.tc:    apl-intro.html

Book.tc.log:	Book.tc
	apl -T $<

EXPO:
	cp	README apl-intro apl-home.css \
		Makefile make_APL_demo make_intro \
		translate.cc ../../../savannah-repo-apl/trunk/web-home/apl/
clean:
	rm translate Book.tc.log Book.tc

