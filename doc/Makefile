all:	mdebs-pretty.txt mdebs-plain.txt mdebs.info mdebs.7

#mdebs.dvi: 	mdebs.sgml
#	sgml2latex --output=dvi mdebs.sgml

#mdebs.ps:	mdebs.sgml
#	sgml2latex --output=ps mdebs.sgml

mdebs.info:	mdebs.sgml
	sgml2info mdebs.sgml

mdebs-pretty.txt:	mdebs.sgml
	sgml2txt mdebs.sgml
	mv mdebs.txt mdebs-pretty.txt

mdebs-plain.txt:	mdebs.sgml
	sgml2txt --filter mdebs.sgml
	mv mdebs.txt mdebs-plain.txt

#mdebs.html:	mdebs.sgml
#	sgml2html -split 0 mdebs.sgml ; cp mdebs.html \
#	  /var/www/mdebs/index.html 

mdebs.7:	mdebs-man.sgml
	sgml2txt --manpage mdebs-man.sgml
	sed -e "s/\.TH \"Mdebs\" 1/\.TH \"Mdebs\" 7/g" mdebs-man.man > mdebs.7
	rm mdebs-man.man

install:
	install mdebs.7 /usr/local/man/man7

clean:
	rm -f *~ core mdebs.{txt,tex,ps,dvi,html,man,info,aux,log} \
	  mdebs-pretty.txt mdebs-plain.txt mdebs.7 mdebs.info

maintainer-clean: clean
