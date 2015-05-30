
all:	APL_demo.html index.html apl-intro.html translate Book.tc.log dummy apl.html

apl.html:	../../doc/apl.html
	make -C ../../doc apl.html
	cp $^ $@

APL_demo.html:	../../workspaces/APL_CGI.apl make_APL_demo
	./make_APL_demo

index.html:	../../workspaces/APL_CGI.apl
	$< > $@

dummy:
	./make_intro apl-intro make_intro translate

apl-intro.html:	apl-intro make_intro translate
	./make_intro $<

translate:  translate.cc
	g++ -g2 -o $@ $< -lapl -L /usr/local/lib/apl

Book.tc:    apl-intro.html

Book.tc.log:	Book.tc
#	apl -T $<

EXPO:
	cp	README apl-intro apl-home.css \
		Makefile make_APL_demo make_intro \
		translate.cc ../../../savannah-repo-apl/trunk/web-home/apl/
	make -C Bits_and_Pieces EXPO

clean:
	rm -f translate Book.tc.log Book.tc

