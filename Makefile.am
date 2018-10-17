SUBDIRS = src
dist_doc_DATA = README example.conf

man_MANS = xchainkeys.1
EXTRA_DIST = xchainkeys.1

install-data-hook : 
	$(SED) -E -i.orig 's|__PREFIX__|@prefix@|g' $(DESTDIR)$(man1dir)/xchainkeys.1
	rm $(DESTDIR)$(man1dir)/*.orig
